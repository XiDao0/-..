#ifndef BRK_SERVICE_DISPATCH_EVENT_SERVICE_H_
#define BRK_SERVICE_DISPATCH_EVENT_SERVICE_H_

#include <map>
#include <vector>
//#include <stdio.h>
#include <algorithm>
#include <queue>

#include "ievent.h"
#include "eventtype.h"
#include "iEventHandler.h"
#include "threadpool/thread_pool.h"
#include "bike.pb.h"
#include "events_def.h"
#include "NetworkInterface.h"


class DispatchMsgService
{
protected:
	DispatchMsgService();

public:
	virtual ~DispatchMsgService();

	virtual BOOL open();
	virtual void close();

	virtual void subscribe(u32 eid, iEventHandler* handle);
	virtual void unsubscribe(u32 eid, iEventHandler* handle);

	virtual i32 enqueue(iEvent* ev);

	// 对事件分发处理
	virtual iEvent* process(const iEvent* ev);

	// 线程池回调函数
	static void svc(void* argv);

	static DispatchMsgService* getInstance();

	iEvent* parseEvent(const char* message, u32 len, u32 eid);

	void handleAllResponseEvents(NetworkInterface* interface);

private:
	thread_pool_t* tp;

	static DispatchMsgService* DMS_;

	typedef std::vector<iEventHandler*> T_EventHandlers;
	typedef std::map<u32, T_EventHandlers> T_EventHandlersMap;
	T_EventHandlersMap subscribers_;

	bool svr_exit_;

	static std::queue<iEvent*>	response_events;
	static pthread_mutex_t		queue_mutex;
};


#endif // !BRK_SERVICE_DISPATCH_EVENT_SERVICE_H_
