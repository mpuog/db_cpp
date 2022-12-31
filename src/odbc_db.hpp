#pragma once

#include "common.hpp"
#include <Windows.h>
#include <sql.h>

using namespace dbpp;

/// Common data/tools for ODBC connection and cursor
class BaseOdbc
{
};

typedef struct STR_BINDING {
    SQLSMALLINT         cDisplaySize;           /* size to display  */
    WCHAR* wszBuffer;             /* display buffer   */
    SQLLEN              indPtr;                 /* size or null     */
    BOOL                fChar;                  /* character col?   */
    struct STR_BINDING* sNext;                 /* linked list      */
} BINDING;

/*******************************************/
/* Macro to call ODBC functions and        */
/* report an error on failure.             */
/* Takes handle, handle type, and stmt     */
/*******************************************/
#if 1
#define TRYODBC(h, ht, x)
#else
#define TRYODBC(h, ht, x)   {   RETCODE rc = x;\
                                if (rc != SQL_SUCCESS) \
                                { \
                                    HandleDiagnosticRecord (h, ht, rc); \
                                } \
                                if (rc == SQL_ERROR) \
                                { \
                                    fwprintf(stderr, L"Error in " L#x L"\n"); \
                                    throw Error("*");  \
                                }  \
                            }

#endif

class SqlHandle
{
	SQLHANDLE handle = SQL_NULL_HANDLE;
	SQLSMALLINT handleType;
public:

	SqlHandle(SqlHandle const&) = delete;
	SqlHandle& operator = (SqlHandle const&) = delete;

	/// to create empty object before SQLAllocHand
	explicit SqlHandle(SQLSMALLINT handleType_)
		: handleType(handleType_) 
	{}

	~SqlHandle()
	{
		if (handle) // if initialized
			SQLFreeHandle(handleType, handle);
	}

	SQLHANDLE * operator &()
	{
		return &handle;
	}

	operator SQLHANDLE () 
	{
		return handle;
	}

};


class OdbcConnection : public BaseConnection, public BaseOdbc
{
	friend class ObbcCursor;
public:

	SqlHandle hEnv;
	SqlHandle hDbc;
public:

	OdbcConnection(std::string const& connectString);
	~OdbcConnection() = default;

	// Inherited via BaseConnection
	BaseCursor* cursor() final;
};

class OdbcCursor : public BaseCursor, public BaseOdbc
{
	OdbcConnection& connection;
public:
	explicit OdbcCursor(OdbcConnection& connection_)
		: connection(connection_)
	{

	}

	int execute_impl(String const& query, InputRow const& data,
		std::deque<ResultRow>& resultTab, ColumnsInfo& columnsInfo) final;
};

