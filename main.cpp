#include <iostream>
#include <unistd.h>
#include <memory>

#include "bike.pb.h"
#include "glo_def.h"
#include "ievent.h"
#include "events_def.h"
#include "user_event_handler.h"
#include "DispatchMsgService.h"
#include "NetworkInterface.h"
#include "iniconfig.h"
#include "Logger.h"
#include "sqlconnection.h"
#include "BusProcessor.h"

using namespace std;

int main(int argc, char** argv) {

	//tutorial::mobile_request msg;
	//msg.set_mobile("1110022");

	//iEvent* ev = new iEvent(EEVENTID_GET_MOBLIE_CODE_REQ, 2);

	//MobileCodeReqEv me("123345+678");
	///me.dump();
	//me.get_mobile();
	//me.get_eid();

	//MobileCodeRspEv mr(200, 6666);
	//mr.dump();

	//UserEventHandler u_h;
	//iEvent* mr1 = u_h.handle(&me);
	//mr1->dump();

	if (argc != 3)
	{
		printf("Please input shbk < config file path > !\n");
		return -1;
	}

	if (!Logger::instance()->init(std::string(argv[2])))
	{
		fprintf(stderr, "init log module failed. \n");
		return -2;
	}

	Iniconfig* config = Iniconfig::getInstance();
	if (!config->loadfile(std::string(argv[1])))
	{
		LOG_ERROR("load %s failed.\n", argv[1]);
		return -3;
	}
	st_env_config conf_args = config->getconfig();

	LOG_INFO("[database] ip: %s port: %d user: %s pwd: %s db: %s [server] port: %d",
		conf_args.db_ip.c_str(), conf_args.db_port, conf_args.db_user.c_str(), conf_args.db_pwd.c_str(), conf_args.db_name.c_str(), conf_args.svr_port);

	MysqlConnection mysqlConn;

	DispatchMsgService* DMS = DispatchMsgService::getInstance();

	DMS->open();

	std::shared_ptr<MysqlConnection> mysqlconn(new MysqlConnection);
	if (!mysqlconn->Init(conf_args.db_ip.c_str(), conf_args.db_port,
		conf_args.db_user.c_str(), conf_args.db_pwd.c_str(), conf_args.db_name.c_str()))
	{
		LOG_ERROR("Database mysql init failed. ");
		return -4;
	}

	BusProcessor busPro(mysqlconn);
	busPro.init();

	NetworkInterface* NTIF = new NetworkInterface();
	NTIF->start(conf_args.svr_port);

	while (1 == 1)
	{
		NTIF->network_event_dispatch();
		sleep(1);
		LOG_DEBUG("network_event_dispatch....");
	}

	sleep(5);
	DMS->close();
	sleep(5);

	return 0;
}