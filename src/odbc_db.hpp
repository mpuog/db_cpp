#pragma once

#include "common.hpp"
#include <Windows.h>
#include <sql.h>

using namespace dbpp;


/// Common data/tools for ODBC connection and cursor
class BaseOdbc
{
};

class OdbcCursor : public BaseCursor, public BaseOdbc
{
public:
	//virtual void callproc(string_t const& proc_name) override
	//{		}

	OdbcCursor() = default;

	void execute_impl(String const& query, InputRow const& data) final
	{
		resultTab = { { null, 21}, {"qu-qu", null} };
	}
};


class OdbcConnection : public BaseConnection, public BaseOdbc
{
	SQLHDBC h;
public:

	OdbcConnection(std::string const& connectString);
	OdbcConnection(OdbcConnection&&) = default;

	// Inherited via BaseConnection
	virtual BaseCursor* cursor() final
	{
		return new OdbcCursor;
	}

};

