#include "DispatchMsgService.h"
#include "NetworkInterface.h"

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static ConnectSession* stat_init(int fd, struct bufferevent* bev) {
	ConnectSession* temp = NULL;
	temp = (ConnectSession*)malloc(sizeof(ConnectSession));

	if (!temp) {
		fprintf(stderr, "malloc failed. reason: %m\n");
		return NULL;
	}

	memset(temp, '\0', sizeof(ConnectSession));
	temp->bev = bev;
}

void session_free(ConnectSession* cs)
{
	if (cs)
	{
		if (cs->read_buf)
		{
			delete[] cs->read_buf;
			cs->read_buf = nullptr;
		}

		if (cs->write_buf)
		{
			delete[] cs->write_buf;
			cs->write_buf = nullptr;
		}
		delete cs;
	}
}

void session_reset(ConnectSession* cs)
{
	if (cs)
	{
		if (cs->read_buf)
		{
			delete[] cs->read_buf;
			cs->read_buf = nullptr;
		}
		if (cs->write_buf)
		{
			delete[] cs->write_buf;
			cs->write_buf = nullptr;
		}
		cs->session_stat = SESSION_STATUS::SS_REQUEST;
		cs->req_stat = MESSAGE_STATUS::MS_READ_HEADER;

		cs->message_len = 0;
		cs->read_message_len = 0;
		cs->read_header_len = 0;
	}
}


NetworkInterface::NetworkInterface()
{
	base_ = nullptr;
	listener_ = nullptr;
}

NetworkInterface::~NetworkInterface()
{
	close();
}

bool NetworkInterface::start(int port)
{
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sockaddr_in));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	base_ = event_base_new();

	listener_= evconnlistener_new_bind(base_, listener_cb, base_,
				LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
				512, (struct sockaddr*)&sin,
				sizeof(struct sockaddr_in));

}

void NetworkInterface::close()
{
	if (base_)
	{
		event_base_free(base_);
		base_ = nullptr;
	}

	if (listener_)
	{
		evconnlistener_free(listener_);
		listener_ = nullptr;
	}
}

void NetworkInterface::listener_cb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* sock, int socklen, void* arg)
{
	struct event_base* base = (struct event_base*)arg;

	LOG_DEBUG("accept a client %d\n", fd);

	struct bufferevent* bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

	ConnectSession* cs = stat_init(fd, bev);
	cs->session_stat = SESSION_STATUS::SS_REQUEST;
	cs->req_stat = MESSAGE_STATUS::MS_READ_HEADER;

	strcpy(cs->remote_ip, inet_ntoa(((sockaddr_in*)sock)->sin_addr));

	LOG_DEBUG("remote ip: %s\n", cs->remote_ip);

	bufferevent_setcb(bev, handle_request, handle_response, handle_error, cs);
	bufferevent_enable(bev, EV_READ | EV_PERSIST);
	bufferevent_settimeout(bev, 10, 10);
	
}

void NetworkInterface::handle_request(bufferevent* bev, void* arg)
{
	ConnectSession* cs = (ConnectSession*)arg;

	if (cs->session_stat != SESSION_STATUS::SS_REQUEST)
	{
		LOG_ERROR("NetworkInterface::handle_request - wrong session state[%d]\n", cs->session_stat);
		return;
	}

	if (cs->req_stat == MESSAGE_STATUS::MS_READ_HEADER)
	{
		i32 len = bufferevent_read(bev, cs->header + cs->read_header_len, MESSAGE_HEADER_LEN - cs->read_header_len);
		cs->read_header_len += len;

		cs->header[cs->read_header_len] = '\0';
		LOG_DEBUG("recv from client <<<< %s\n", cs->header);

		if (cs->read_header_len == MESSAGE_HEADER_LEN)
		{
			if (!strncmp(cs->header, MESSAGE_HEADER_ID, strlen(MESSAGE_HEADER_ID)))
			{
				cs->eid = *(u16*)(cs->header + 4);
				cs->message_len = *(u32*)(cs->header + 6);

				LOG_DEBUG("NetworkInterface::handle_request - read [%d] bytes in header, message len: [%d]\n", cs->read_header_len, cs->message_len);

				if (cs->message_len < 1 || cs->message_len > MAX_MESSION_LEN)
				{
					LOG_ERROR("NetworkInterface::handle_request - message_len is too bigger: %u\n", cs->message_len);
					bufferevent_free(bev);
					session_free(cs);
					return;
				}

				cs->req_stat = MESSAGE_STATUS::MS_READ_MESSAGE;
				cs->read_buf = new char[cs->message_len];
				cs->read_message_len = 0;

			}
			else
			{
				LOG_ERROR("NetworkInterface::handle_request - : Invalid requset from %s\n", cs->remote_ip);
				// Ö±½Ó¹Ø±Õ
				bufferevent_free(bev);
				session_free(cs);
				return;
			}
		}
	}

	if (cs->req_stat == MESSAGE_STATUS::MS_READ_MESSAGE && evbuffer_get_length(bufferevent_get_input(bev)) > 0)
	{
		i32 len = bufferevent_read(bev, cs->read_buf + cs->read_message_len, cs->message_len - cs->read_message_len);
		cs->read_message_len += len;
		LOG_DEBUG("NetworkInterface::handle_request -message len: [%d], read [%d] bytes, eid: %u\n", cs->message_len, cs->read_message_len, cs->eid);

		if (cs->read_message_len == cs->message_len)
		{
			cs->session_stat = SESSION_STATUS::SS_RESPONSE;
			iEvent* ev =  DispatchMsgService::getInstance()->parseEvent(cs->read_buf, cs->message_len, cs->eid);

			delete[] cs->read_buf;
			cs->read_message_len = 0;
			cs->read_buf = nullptr;

			if (ev)
			{
				ev->set_args(cs);
				DispatchMsgService::getInstance()->enqueue(ev);
			}
			else
			{
				LOG_ERROR("NetworkInterface::handle_request - : ev is null, remote ip: %s\n", cs->remote_ip);
			}
		}
	}

}

void NetworkInterface::handle_response(bufferevent* bev, void* arg)
{
	
}

void NetworkInterface::handle_error(bufferevent* bev, short event, void* arg)
{
	ConnectSession* cs = (ConnectSession*)arg;

	LOG_DEBUG("NetworkInterface::handle_error ...\n");

	if (event & BEV_EVENT_EOF)
	{
		LOG_DEBUG("connection closed\n");
	}
	else if ((event & BEV_EVENT_TIMEOUT) && (event & BEV_EVENT_READING))
	{
		LOG_WARN("NetworkInterface::reading timeout ...\n");
	}
	else if ((event & BEV_EVENT_TIMEOUT) && (event & BEV_EVENT_WRITING))
	{
		LOG_WARN("NetworkInterface::writting timeout ...\n");
	}
	else if (event & BEV_EVENT_ERROR)
	{
		LOG_WARN("NetworkInterface::some other error ...\n");
	}

	bufferevent_free(bev);
	session_free(cs);

}

void NetworkInterface::network_event_dispatch()
{
	event_base_loop(base_, EVLOOP_NONBLOCK);

	DispatchMsgService::getInstance()->handleAllResponseEvents(this);
}

void NetworkInterface::send_response_message(ConnectSession* cs)
{
	if (cs->response = NULL)
	{
		bufferevent_free(cs->bev);
		if (cs->request)
		{
			delete cs->request;
		}
		session_free(cs);
	}
	else
	{
		bufferevent_write(cs->bev, cs->write_buf, cs->message_len + MESSAGE_HEADER_LEN);
		session_reset(cs);
	}

}
