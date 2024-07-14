#ifndef DATASTORE_MYSQL_CONNECTION_H_
#define DATASTORE_MYSQL_CONNECTION_H_

#include <mysql/mysql.h>
#include <string.h>
#include <mysql/errmsg.h>
#include <assert.h>

#include "glo_def.h"

class SqlRecordSet
{
public:
	SqlRecordSet() : m_pRes_(NULL){

	}

	explicit SqlRecordSet(MYSQL_RES* pRes)
	{
		m_pRes_ = pRes;
	}

	MYSQL_RES* MysqlRes()
	{
		return m_pRes_;
	}

	~SqlRecordSet()
	{
		if (m_pRes_)
		{
			mysql_free_result(m_pRes_);
		}
	}

	inline void SetResult(MYSQL_RES* pRes)
	{
		assert(m_pRes_ == NULL);
		if (m_pRes_)
		{
			LOG_WARN("the MYSQL_RES has already stored result");
		}
		m_pRes_ = pRes;
	}

	inline MYSQL_RES* GetResult()
	{
		return m_pRes_;
	}

	void FetchRow(MYSQL_ROW row)
	{
		row = mysql_fetch_row(m_pRes_);
	}

	inline i32 GetRowCount()
	{
		return m_pRes_->row_count;
	}

private:
	MYSQL_RES* m_pRes_;
};

class MysqlConnection
{
public:
	MysqlConnection();
	~MysqlConnection();

	MYSQL* Mysql()
	{
		return mysql_;
	}

	bool Init(const char* szHost, int port, const char* szUser, const char* szPasswd, const char* szDb);

	bool Execute(const char* szSql);

	bool Execute(const char* szSql, SqlRecordSet& recordSet);

	int EscapeString(const char* pSrc, int nSrclen, char* pDest);

	void close();

	const char* GetErrInfo();

	void Reconnect();

private:
	MYSQL* mysql_;

};


#endif // !DATASTORE_MYSQL_CONNECTION_H_
