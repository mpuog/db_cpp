#pragma once

#include "common.hpp"
#include <Windows.h>
#include <sql.h>

using namespace dbpp;

#define PRINT1(v) std::cout << #v << "="<< v << "\n";

typedef struct STR_BINDING {
    SQLSMALLINT         cDisplaySize;           /* size to display  */
    WCHAR* wszBuffer;             /* display buffer   */
    SQLLEN              indPtr;                 /* size or null     */
    BOOL                fChar;                  /* character col?   */
    struct STR_BINDING* sNext;                 /* linked list      */
} BINDING;


//void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);
void AllocateBindings(HSTMT hStmt, SQLSMALLINT cCols, BINDING **ppBinding, SQLSMALLINT *pDisplay);
void SetConsole(DWORD dwDisplaySize, BOOL fInvert);
void DisplayTitles(HSTMT hStmt, DWORD cDisplaySize, BINDING *pBinding);
void DisplayResults(HSTMT hStmt, SQLSMALLINT cCols);

/// Common data/tools for ODBC connection and cursor
class BaseOdbc
{
};

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

	SqlHandle(SQLSMALLINT handleType_, SQLHANDLE inputHandle, std::string const& type="")
		: handleType(handleType_) 
	{
		if (SQLAllocHandle(handleType, inputHandle, &handle) == SQL_ERROR)
			throw Error("Unable to allocate handle " + type);
	}

	~SqlHandle()
	{
		if (handle) // if initialized
			SQLFreeHandle(handleType, handle);
	}

	SQLHANDLE * operator &() {
		return &handle;	}
	operator SQLHANDLE () {
		return handle; 	}
	SQLSMALLINT Type() const { 
		return handleType; };
};

#define CONSTRUCT_HANDLE_WITH_TYPE(h, type, inpH) h(type, inpH, #type) 

class ErrorODBC : public Error
{
public:
	ErrorODBC(std::string const& message, RETCODE retCode);
};

std::string GetDiagnostic(SqlHandle &handle, RETCODE retCode);

void CheckResultCode(SqlHandle &h, RETCODE rc, std::string const& errMsg="Error");

/* Macro to call ODBC functions and report an error on failure. Takes handle, handle type, and stmt */
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
                                    //fwprintf(stderr, L"Error in " L#x L"\n"); \
                                    throw Error("*");  \
                                }  \
                            }
#endif


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

	int get_execute_result(SqlHandle &hStmt, SQLSMALLINT numResults,
						   std::deque<ResultRow> &resultTab, ColumnsInfo &columnsInfo);
	ResultRow get_row(SqlHandle &hStmt, SQLSMALLINT numCols);

public:
	explicit OdbcCursor(OdbcConnection& connection_)
		: connection(connection_)
	{
	}

	int execute_impl(String const& query, InputRow const& data,
		std::deque<ResultRow>& resultTab, ColumnsInfo& columnsInfo) final;
};

