#include "user_event_handler.h"
#include "glo_def.h"
#include "DispatchMsgService.h"
#include "user_service.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <unistd.h>

UserEventHandler::UserEventHandler() : iEventHandler("UserEventHandler")
{
	DispatchMsgService::getInstance()->subscribe(EEVENTID_GET_MOBLIE_CODE_REQ, this);
	DispatchMsgService::getInstance()->subscribe(EEVENTID_GET_MOBLIE_CODE_RSP, this);
	DispatchMsgService::getInstance()->subscribe(EEVENTID_LOGIN_REQ, this);
	thread_mutex_create(&pm_);
}

UserEventHandler::~UserEventHandler()
{
	DispatchMsgService::getInstance()->unsubscribe(EEVENTID_GET_MOBLIE_CODE_REQ, this);
	DispatchMsgService::getInstance()->unsubscribe(EEVENTID_GET_MOBLIE_CODE_RSP, this);
	DispatchMsgService::getInstance()->unsubscribe(EEVENTID_LOGIN_REQ, this);

	thread_mutex_destroy(&pm_);
}

iEvent* UserEventHandler::handle(const iEvent* ev) {

	if (ev == NULL)
	{
		// LOG_ERROR("input ev is NULL");
		printf("input ev is NULL\n");
	}
	//printf("eid: %u\n");

	u32 eid = ev->get_eid();

	//printf("eid: %u\n", eid);

	//(eid == EEVENTID_GET_MOBLIE_CODE_REQ) ? DEBUG("1") : DEBUG("2");

	if (eid == EEVENTID_GET_MOBLIE_CODE_REQ)
	{
		//LOG_DEBUG(".....----\n");
		return handle_mobile_code_req((MobileCodeReqEv*) ev);
	}
	else if (eid == EEVENTID_LOGIN_REQ)
	{
		return handle_login_req((LoginReqEv*) ev);
	}
	else if (eid == EEVENTID_RECHARGE_REQ)
	{
		// return handle_recharge_req((RechargeEv*) ev);
	}
	else if (eid == EEVENTID_GET_ACCOUNT_BALANCE_REQ)
	{
		// return handle_get_account_balance_req((GerAccountBalanceEv*) ev);
	}
	else if (eid == EEVENTID_LIST_ACCOUNT_RECORDS_REQ)
	{
		// return handle_list_account_records_req((ListAccountRecordsReqEv*) ev);
	}
	
}

MobileCodeRspEv* UserEventHandler::handle_mobile_code_req(MobileCodeReqEv* ev) {
	i32 icode = 0;
	mobile_ = ev->get_mobile();
	printf("try to get mobile phone %s\n", mobile_.c_str());

	icode = code_gen();
	thread_mutex_lock(&pm_);
	m2c_[mobile_] = icode;
	thread_mutex_unlock(&pm_);
	printf("mobile: %s, code: %d\n", mobile_.c_str(), icode);

	MobileCodeRspEv* ret = new MobileCodeRspEv(200, icode);
	ret->set_eid(EEVENTID_GET_MOBLIE_CODE_RSP);

	return ret;
}

LoginResEv* UserEventHandler::handle_login_req(LoginReqEv* ev)
{
	LoginResEv* loginEv = nullptr;

	std::string mobile = ev->get_mobile();
	i32 icode = ev->get_icode();

	LOG_DEBUG("try to handle login ev, mobile = %s, icode = %d", mobile.c_str(), icode);

	thread_mutex_lock(&pm_);
	auto iter = m2c_.find(mobile);

	if ((iter != m2c_.end() && (iter->second != icode)) || iter == m2c_.end())
	{
		loginEv =  new LoginResEv(ERRC_INVALID_DATA);
	}
	thread_mutex_unlock(&pm_);

	if (loginEv) return loginEv;

	// 查找用户数据是否存在
	std::shared_ptr<MysqlConnection> mysqlconn(new MysqlConnection);

	st_env_config config = Iniconfig::getInstance()->getconfig();
	if (!mysqlconn->Init(config.db_ip.c_str(), config.db_port, config.db_user.c_str(), config.db_pwd.c_str(), config.db_name.c_str()))
	{
		LOG_ERROR("UserEventHandler::handle_login_req - Database init failed");
		return new LoginResEv(ERRC_PROCCESS_FAILED);
	}
	UserService us(mysqlconn);
	bool result = false;

	if (!us.exist(mobile))
	{
		result = us.insert(mobile);
		if (!result)
		{
			LOG_ERROR("insert user(%s) to db failed", mobile);
			return new LoginResEv(ERRC_PROCCESS_FAILED);
		}
	}

	return new LoginResEv(ERRC_SUCCESS);
}

i32 UserEventHandler::code_gen() {
	i32 code = 0;
	srand((unsigned int)time(NULL));

	code = (unsigned int)(rand() % (999999 - 100000) + 100000);

	return code;
}