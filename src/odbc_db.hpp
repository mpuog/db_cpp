#pragma once

#include "common.hpp"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <sql.h>
#include <iostream>

using namespace dbpp;

#define PRINT1(v) std::cout << #v << "="<< v << "\n";

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
	SqlHandle(SqlHandle &&) = delete;
	SqlHandle& operator = (SqlHandle &&);

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
    	if (SQL_NULL_HANDLE != handle) // if initialized
		{
			SQLFreeHandle(handleType, handle);
			handle = SQL_NULL_HANDLE;
		}
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
	//ErrorODBC(std::string const& message, RETCODE retCode);
	ErrorODBC(SqlHandle &h, RETCODE retCode, std::string const& sAddMsg="");
};

std::string GetDiagnostic(SqlHandle &handle, RETCODE retCode);

void CheckResultCode(SqlHandle &h, RETCODE rc, std::string const& errMsg="Error");

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

	bool autocommit() final;
	void autocommit(bool autocommitFlag) final;
	void commit() final;
	void rollback() final;
};

class OdbcCursor : public BaseCursor, public BaseOdbc
{
	struct OneColumnInfo
	{
		// SQL_DESC_CONCISE_TYPE ?? or SQL_DESC_TYPE
		SQLLEN columnType;  ///< column type id
		String columnTypeName;  ///< column type name
		size_t indexVariant;  ///< index in ResultCell variant if data isn't null
		SQLLEN getDataType;  ///< type for using in SQLGetData function


	public:
		OneColumnInfo(SQLLEN type, const String& name);

	};

	struct BindOneParam
	{
		SqlHandle& hStmt;
		SQLUSMALLINT nParam;
		SQLLEN &cb;
	public:
		//BindOneParam(SqlHandle& hStmt) : hStmt(hStmt) {}
		void operator()(const Null&);
		void operator()(const int& i);
		void operator()(const double& d);
		void operator()(const String& s);
		void operator()(const Blob& b);
	};

	std::vector < OneColumnInfo>  columnsInfoODBC;

	OdbcConnection& connection;
    SqlHandle hStmt;	

	int get_execute_result(SQLSMALLINT numResults,
						   std::deque<ResultRow> &resultTab);
	void get_columns_info(SQLSMALLINT numResults, ColumnsInfo& columnsInfo);
	ResultRow get_row(SQLSMALLINT numCols);
	ResultCell get_cell(SQLSMALLINT nCol);

public:
	explicit OdbcCursor(OdbcConnection& connection_)
		: connection(connection_), hStmt(SQL_HANDLE_STMT)
	{
	}

	int execute_impl(String const& query, InputRow const& data,
		std::deque<ResultRow>& resultTab, ColumnsInfo& columnsInfo) final;
};

