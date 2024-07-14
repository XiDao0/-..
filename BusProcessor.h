#ifndef BRKS_BUS_MAIN_H_
#define BRKS_BUS_MAIN_H_

#include "user_event_handler.h"
#include "sqlconnection.h"

class BusProcessor
{
public:
	BusProcessor(std::shared_ptr<MysqlConnection> conn);
	bool init();

	~BusProcessor();

private:
	std::shared_ptr<MysqlConnection> mysqlconn_;
	std::shared_ptr<UserEventHandler> ueh_;
};


#endif // !BRKS_BUS_MAIN_H_

