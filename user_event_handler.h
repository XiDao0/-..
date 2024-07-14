#ifndef BRKS_BUS_USERM__HANDLER_H_
#define BRKS_BUS_USERM__HANDLER_H_

#include "glo_def.h"
#include "events_def.h"
#include "user_service.h"
#include "iniconfig.h"
#include "iEventHandler.h"
#include "threadpool/thread.h"

#include <string>
#include <map>
#include <memory>

class UserEventHandler : public iEventHandler
{
public:
	UserEventHandler();
	virtual iEvent* handle(const iEvent* ev);
	virtual ~UserEventHandler();
	
private:
	MobileCodeRspEv* handle_mobile_code_req(MobileCodeReqEv* ev);
	LoginResEv* handle_login_req(LoginReqEv* ev);
	i32 code_gen();

private:
	std::string mobile_;
	std::map<std::string, i32> m2c_;
	pthread_mutex_t pm_;

};

#endif // !BRKS_BUS_USERM__HANDLER_H_

