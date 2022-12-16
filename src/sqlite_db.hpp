#pragma once
#include "sqlite/sqlite3.h"
#include "common.hpp"
#include <functional>
using namespace dbpp;

void finalize_with_check(sqlite3_stmt* stmt);

class BaseSqlite
{
protected:
	// ==== data ====
	std::shared_ptr<sqlite3> db;

	// ==== functions ====
	std::shared_ptr<sqlite3_stmt> single_sqlite_stmt(std::string const& sql)
	{
		sqlite3_stmt* stmt;
		int rc = sqlite3_prepare_v2(db.get(), sql.c_str(), int(sql.size() + 1), &stmt, nullptr);
		if (rc != SQLITE_OK)
			throw Error("Bad SQL");

		//std::unique_ptr<sqlite3_stmt, std::function<void(sqlite3_stmt*)> > zz(stmt, finalize_with_check);
		return std::shared_ptr<sqlite3_stmt>(stmt, finalize_with_check);
	}

	void single_step(std::string const& sql)
	{
		(void)sqlite3_step(single_sqlite_stmt(sql).get());
	}
	
	BaseSqlite(std::shared_ptr<sqlite3> db_=std::shared_ptr<sqlite3>())
		: db(db_)
	{}
};


class SqliteCursor : public BaseCursor, public BaseSqlite
{
// ==== functions ===
    ResultRow GetRow(sqlite3_stmt* pStmt);
public:

	//virtual void callproc(string_t const& proc_name) override
	//{		}

	SqliteCursor(std::shared_ptr<sqlite3> db);

	void execute_impl(String const& query, InputRow const& data) override;
};


class SqliteConnection : public BaseConnection, public BaseSqlite
{
	
public:

	~SqliteConnection();

	SqliteConnection(std::string const& connectString, std::string const& addParams);
	SqliteConnection(SqliteConnection&&) = default;

    bool autocommit() final;
    void autocommit(bool autocommitFlag) final;
    void commit() final;
    void rollback() final;

	BaseCursor *cursor() final {
		return new SqliteCursor(db);
	}
};