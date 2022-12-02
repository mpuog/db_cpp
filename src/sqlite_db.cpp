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

// =============== SqliteCursor =====================

SqliteCursor::SqliteCursor(std::shared_ptr<sqlite3> db_)
	: db(db_)
{
}

static int callback(void* data, int argc, char** argv, char** azColName)
{
	SqliteCursor *cursor = static_cast<SqliteCursor*>(data);
	ResultRow resultRow;
	bool flagFirst = cursor->columns.empty();
	for (int i = 0; i < argc; i++) {
		if (argv[i])
			resultRow.emplace_back(std::string(argv[i]));
		else
			resultRow.emplace_back(null);
		if (flagFirst)
			cursor->columns.emplace_back(azColName[i]);
	}
	cursor->resultTab.push_back(std::move(resultRow));
	return 0;
}


void SqliteCursor::execute_impl(String const& sql, InputRow const& data)
{
	//execute(sql, data);
	if (sql.substr(0,6) == "SELECT")
		execute(sql, data);
	else
		execute(sql, data);

}

// test-only implemantation by call sqlite3_exec()
void SqliteCursor::execute0(const dbpp::String& sql, const dbpp::InputRow& data)
{
	resultTab.clear();

	char* messaggeError;
	int exit = sqlite3_exec(
		db.get(), sql.c_str(), callback, this, &messaggeError);
	if (exit) {
		Error error(messaggeError);
		sqlite3_free(messaggeError);
		throw error;
	}
}

void SqliteCursor::execute(String const& query, InputRow const& data)
{
	const char* zSql = query.c_str();
	int rc = SQLITE_OK;         /* Return code */
	const char* zLeftover;      /* Tail of unprocessed SQL */
	sqlite3_stmt* pStmt = 0;    /* The current SQL statement */
    char** pzErrMsg;
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
			resultTab.push_back(std::move(row));
		}
		rc = sqlite3_finalize(pStmt);
		zSql = zLeftover;
    }  // Cycle for all SQL single statements
}
