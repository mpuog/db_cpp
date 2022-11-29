#pragma once
#include "sqlite3.h"
#include <dbpp.h>
#include <iostream>
using namespace dbpp;

class SqliteCursor : public BaseCursor
{
	sqlite3* db;

	void execute0(const dbpp::String& sql, const dbpp::InputRow& data);
	void execute(String const& query, InputRow const& data);
public:

	//virtual void callproc(string_t const& proc_name) override
	//{		}

	SqliteCursor(sqlite3* db);

	void execute_impl(String const& query, InputRow const& data) override;
};


class SqliteConnection : public BaseConnection
{
	
public:

	sqlite3* db = nullptr;

	~SqliteConnection();

	SqliteConnection(std::string const& connect_string);
	SqliteConnection(SqliteConnection&&) = default;

	virtual std::unique_ptr<BaseCursor> cursor() override
	{
		return std::unique_ptr<BaseCursor>(new SqliteCursor(db));
	}
};