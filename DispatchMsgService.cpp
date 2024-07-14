#include "DispatchMsgService.h"
#include <algorithm>

DispatchMsgService* DispatchMsgService::DMS_ = nullptr;
pthread_mutex_t DispatchMsgService::queue_mutex;
std::queue<iEvent*> DispatchMsgService::response_events;

DispatchMsgService::DispatchMsgService()
{
	tp = NULL;

}

DispatchMsgService::~DispatchMsgService()
{
	
}

BOOL DispatchMsgService::open() 
{

	thread_mutex_create(&queue_mutex);
	svr_exit_ = FALSE;
	tp = thread_pool_init();

	return tp ? TRUE : FALSE;
}

void DispatchMsgService::close()
{
	thread_mutex_destroy(&queue_mutex);
	svr_exit_ = TRUE;
	thread_pool_destroy(tp);
	subscribers_.clear();

	tp = NULL;
	
}

void DispatchMsgService::subscribe(u32 eid, iEventHandler* handle)
{
	LOG_DEBUG("DispatchMsgService::subscribe eid: %u\n", eid);
	T_EventHandlersMap::iterator iter = subscribers_.find(eid);
	if (iter != subscribers_.end())
	{
		T_EventHandlers::iterator hdl_iter = std::find(iter->second.begin(), iter->second.end(), handle);
		if (hdl_iter == iter->second.end())
		{
			iter->second.push_back(handle);
			LOG_DEBUG("%d push_back\n", eid);
		}
	}
	else 
	{
		subscribers_[eid].push_back(handle);
		LOG_DEBUG("%d push_back..\n", eid);
	}
}

void DispatchMsgService::unsubscribe(u32 eid, iEventHandler* handle)
{
	T_EventHandlersMap::iterator iter = subscribers_.find(eid);
	if (iter != subscribers_.end())
	{
		T_EventHandlers::iterator hdl_iter = std::find(iter->second.begin(), iter->second.end(), handle);
		if (hdl_iter != iter->second.end())
		{
			iter->second.erase(hdl_iter);
		}
	}
}

i32 DispatchMsgService::enqueue(iEvent* ev)
{
	if (ev == NULL) 
	{
		LOG_ERROR("ev null \n");
		return -1;
	}

	thread_task_t* task = thread_task_alloc(0);

	task->handler = DispatchMsgService::svc;
	task->ctx = ev;

	return thread_task_post(tp, task);
}

iEvent* DispatchMsgService::process(const iEvent* ev)
{
	ev->dump();
	LOG_DEBUG("DispatchMsgService::process -ev: %p\n", ev);
	if (ev == NULL)
	{
		return NULL;
	}

	u32 eid = ev->get_eid();

	LOG_DEBUG("DispatchMsgService::process -eid: %u\n", eid);
	
	if (eid == EEVENTID_UNKOWN)
	{
		LOG_WARN("DispatchMseService: Unkown event id %d\n", eid);
		return NULL;
	}

	//LOG_DEBUG("dispatch ev: %d\n", eid);

	T_EventHandlersMap::iterator handlers = subscribers_.find(eid);
	if (handlers == subscribers_.end())
	{
		LOG_WARN("DispatchMsgService:: no any event handler subcribed %d\n", eid);
		return NULL;
	}

	iEvent* rsp = NULL;
	for (auto iter = handlers->second.begin(); iter != handlers->second.end(); ++iter)
	{
		iEventHandler* handler = *iter;
		LOG_DEBUG("DispatcheMsgService:: get handler: %s\n", handler->get_name().c_str());
		rsp = handler->handle(ev);
	}

	return rsp;
}

void DispatchMsgService::svc(void* argv)
{
	DispatchMsgService* dms = DispatchMsgService::getInstance();
	iEvent* ev = (iEvent*)argv;
	if (!dms->svr_exit_)
	{
		LOG_DEBUG("DispatchMsgService::srv...\n");
		
		iEvent* rsp = dms->process(ev);

		//delete ev;
		if (rsp)
		{
			rsp->dump();
			rsp->set_args(ev->get_args());
		}
		else 
		{
			rsp = new ExitRspEv();
			rsp->set_args(ev->get_args());
		}

		thread_mutex_lock(&queue_mutex);
		//LOG_DEBUG("handle All response ..mutex..debug\n");
		response_events.push(rsp);
		thread_mutex_unlock(&queue_mutex);

	}
}

DispatchMsgService* DispatchMsgService::getInstance()
{
	if (DMS_ == nullptr)
	{
		DMS_ = new DispatchMsgService();
	}

	return DMS_;
}

iEvent* DispatchMsgService::parseEvent(const char* message, u32 len, u32 eid)
{
	if (!message)
	{
		LOG_ERROR("NetworkInterface::parseEvent - : message is null[%u]: %s.\n", eid);
	}

	if (eid == EEVENTID_GET_MOBLIE_CODE_REQ)
	{
		tutorial::mobile_request mr;
		if (mr.ParseFromArray(message, len))
		{
			MobileCodeReqEv* ev = new MobileCodeReqEv(mr.mobile());
			ev->set_eid(eid);
			//ev->dump();
			return ev;
		}
	}else if (eid == EEVENTID_LOGIN_REQ)
	{
		tutorial::login_request lr;
		if (lr.ParseFromArray(message, len))
		{
			LoginReqEv* ev = new LoginReqEv(lr.mobile(), lr.icode());
			ev->set_eid(eid);
			//ev->dump();
			return ev;
		}
	}

	return nullptr;
}

void DispatchMsgService::handleAllResponseEvents(NetworkInterface* inter)
{
	bool done = false;

	while (!done)
	{
		iEvent* ev = nullptr;
		thread_mutex_lock(&queue_mutex);
		if (!response_events.empty())
		{
			LOG_DEBUG("handle All response ....debug\n");
			ev = response_events.front();
			response_events.pop();
		}
		else
		{
			done = true;
		}
		thread_mutex_unlock(&queue_mutex);
		//u32 ret = ev->get_eid();
		//LOG_DEBUG("eid: [%d]\n", ev->get_eid());
		if (!done)
		{
			if (ev->get_eid() == EEVENTID_GET_MOBLIE_CODE_RSP)
			{
				//MobileCodeRspEv* mcre = static_cast<MobileCodeRspEv*>(ev);
				ConnectSession* cs = (ConnectSession*)ev->get_args();
				cs->response = ev;

				// 系列化请求数据
				cs->message_len = ev->ByteSize();
				cs->write_buf = new char[cs->message_len + MESSAGE_HEADER_LEN];

				memcpy(cs->write_buf, MESSAGE_HEADER_ID, 4);
				*(u16*)(cs->write_buf + 4) = EEVENTID_GET_MOBLIE_CODE_RSP;
				*(i32*)(cs->write_buf + 6) = cs->message_len;

				ev->SerializeToArray(cs->write_buf + MESSAGE_HEADER_LEN, cs->message_len);

				inter->send_response_message(cs);
			}
			else if (ev->get_eid() == EEVENTID_EIXT_RSP)
			{
				//MobileCodeRspEv* mcre = static_cast<MobileCodeRspEv*>(ev);
				ConnectSession* cs = (ConnectSession*)ev->get_args();
				inter->send_response_message(cs);
			}
			else if (ev->get_eid() == EEVENTID_LOGIN_RSP)
			{
				//LoginResEv* lgre = static_cast<LoginResEv*>(ev);
				LOG_DEBUG("DispatchMsgService::handleAllResponseEvent - id EEVENTID_LOGIN_RSP\n");

				ConnectSession* cs = (ConnectSession*)ev->get_args();
				cs->response = ev;

				cs->message_len = ev->ByteSize();
				cs->write_buf = new char[cs->message_len + MESSAGE_HEADER_LEN];

				memcpy(cs->write_buf, MESSAGE_HEADER_ID, 4);
				*(u16*)(cs->write_buf + 4) = EEVENTID_LOGIN_RSP;
				*(i32*)(cs->write_buf + 6) = cs->message_len;

				ev->SerializeToArray(cs->write_buf + MESSAGE_HEADER_LEN, cs->message_len);

				inter->send_response_message(cs);
			}
		}
	}

}

