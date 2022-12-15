#pragma once
#include "sqlite/sqlite3.h"
#include "common.hpp"
using namespace dbpp;

class SqliteCursor : public BaseCursor
{
// ==== data ====
	std::shared_ptr<sqlite3> db;
// ==== functions ===
    ResultRow GetRow(sqlite3_stmt* pStmt);
public:

	//virtual void callproc(string_t const& proc_name) override
	//{		}

	SqliteCursor(std::shared_ptr<sqlite3> db);

	void execute_impl(String const& query, InputRow const& data) override;
};


class SqliteConnection : public BaseConnection
{
	
public:

	std::shared_ptr<sqlite3> db;

	~SqliteConnection();

	SqliteConnection(std::string const& connectString, std::string const& addParams);
	SqliteConnection(SqliteConnection&&) = default;

	virtual BaseCursor *cursor() override
	{
		return new SqliteCursor(db);
	}
};