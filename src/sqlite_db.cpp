#include "sqlite_db.hpp"
#include <cstring>
// =====================   SqliteConnection =====================

SqliteConnection::~SqliteConnection()
{
	//sqlite3_close(db);
}

SqliteConnection::SqliteConnection(
	std::string const& connectString, std::string const& addParams)
{
	sqlite3 *tmp;
	int exit = sqlite3_open(connectString.c_str(), &tmp);
	db.reset(tmp, sqlite3_close);

	if (exit) {
		throw Error(sqlite3_errmsg(db.get()));
	}

	/// @todo parsing and tuning addParams
}


bool SqliteConnection::autocommit() 
{
    return !!sqlite3_get_autocommit(db.get());
}

void SqliteConnection::autocommit(bool autocommitFlag) 
{
    // Do smth if status need to change
    bool currentAutoCommit = autocommit();
    if (currentAutoCommit && !autocommitFlag)
    {
    	sqlite3_stmt* pStmt;
      	int rc = sqlite3_prepare_v2(db.get(), "BEGIN", -1, &pStmt, nullptr);
        rc = sqlite3_step(pStmt);
    }
    else if (!currentAutoCommit && autocommitFlag)
    {
    	sqlite3_stmt* pStmt;
      	int rc = sqlite3_prepare_v2(db.get(), "COMMIT", -1, &pStmt, nullptr);
        rc = sqlite3_step(pStmt);
    }
}

void SqliteConnection::commit() 
{
    if (!autocommit())
    {
    	sqlite3_stmt* pStmt;
      	int rc = sqlite3_prepare_v2(db.get(), "COMMIT;", -1, &pStmt, nullptr);
        rc = sqlite3_step(pStmt);
    }
} 

void SqliteConnection::rollback() 
{
    if (!autocommit())
    {
    	sqlite3_stmt* pStmt;
      	int rc = sqlite3_prepare_v2(db.get(), "ROLLBACK", -1, &pStmt, nullptr);
        rc = sqlite3_step(pStmt);
    }
    /*
    if (!sqlite3_get_autocommit(self->db)) {
        int rc;

        Py_BEGIN_ALLOW_THREADS
        sqlite3_stmt *statement;
        rc = sqlite3_prepare_v2(self->db, "ROLLBACK", 9, &statement, NULL);
        if (rc == SQLITE_OK) {
            (void)sqlite3_step(statement);
            rc = sqlite3_finalize(statement);
        }
        Py_END_ALLOW_THREADS

        if (rc != SQLITE_OK) {
            (void)_pysqlite_seterror(self->state, self->db);
            return NULL;
        }

    }

*/
} 



// =============== SqliteCursor =====================

SqliteCursor::SqliteCursor(std::shared_ptr<sqlite3> db_)
	: db(db_)
{
}


ResultRow SqliteCursor::GetRow(sqlite3_stmt* pStmt)
{
    ResultRow row;
    int nData = sqlite3_data_count(pStmt);
    for (int i = 0; i < nData; i++)
    {
        int type = sqlite3_column_type(pStmt, i);
        switch (type)
        {
        case SQLITE_INTEGER: {
            row.emplace_back(sqlite3_column_int(pStmt, i));
            break; }
        case SQLITE_FLOAT: {
            row.emplace_back(sqlite3_column_double(pStmt, i));
            break; }
        case SQLITE_BLOB: {
            int n = sqlite3_column_bytes(pStmt, i);
            BLOB blob(n);
            std::memcpy(&blob[0], sqlite3_column_blob(pStmt, i), n);
            row.push_back(std::move(blob));
            break; }
        case SQLITE_NULL: {
            row.emplace_back(null);
            break; }
        case SQLITE_TEXT: {
            row.emplace_back(String(
                (char*)sqlite3_column_text(pStmt, i)));
            break; }
        default:
            break;
        }

    }
    return row;
}


void SqliteCursor::execute_impl(String const& sql, InputRow const& data)
{
	const char* zSql = sql.c_str();
	int rc = SQLITE_OK;         /* Return code */
	const char* zLeftover;      /* Tail of unprocessed SQL */
	sqlite3_stmt* pStmt = 0;    /* The current SQL statement */
    //char** pzErrMsg;
    void* pArg = this;  // FIXME remove

	// FIXME checking  sqlite3_finalize()

	// Cycle for SQL single statements
	while (rc == SQLITE_OK && zSql[0]) {
		int nCol = 0;
		char** azVals = 0;
		pStmt = nullptr;
		rc = sqlite3_prepare_v2(db.get(), zSql, -1, &pStmt, &zLeftover);
		//std::shared_ptr<sqlite3_stmt> stmtGuard = std::make_shared<sqlite3_stmt>(pStmt, sqlite3_finalize);
		if (rc != SQLITE_OK) {
			continue;
		}
		if (!pStmt) {
			/* this happens for a comment or white-space */
			zSql = zLeftover;
			continue;
		}

		// FIXME bind input data
		
		// Cycle for result tab
		for (;;)
		{
			rc = sqlite3_step(pStmt);
			if (rc != SQLITE_ROW)
				break;
			if (columns.empty())  // fill columns names in first step
			{
				int nCol = sqlite3_column_count(pStmt);
				for (int i = 0; i < nCol; i++) 
					columns.emplace_back(sqlite3_column_name(pStmt, i));
			}
			resultTab.push_back(std::move(GetRow(pStmt)));
		}
		rc = sqlite3_finalize(pStmt);
		zSql = zLeftover;
    }  // Cycle for all SQL single statements

    // FIXME check RC and throw exception
}
