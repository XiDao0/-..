#include "BusProcessor.h"
#include "SqlTables.h"

BusProcessor::BusProcessor(std::shared_ptr<MysqlConnection> conn)
	: mysqlconn_(conn), ueh_(new UserEventHandler())
{

}

bool BusProcessor::init()
{
	SqlTables tables(mysqlconn_);
	tables.CreateUserInfo();
	tables.CreateBikeTable();

	return true;
}

BusProcessor::~BusProcessor()
{
	ueh_.reset();
}
