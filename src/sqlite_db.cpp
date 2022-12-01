#include "sqlite_db.hpp"

// =====================   SqliteConnection =====================

SqliteConnection::~SqliteConnection()
{
	sqlite3_close(db);
}

SqliteConnection::SqliteConnection(std::string const& connect_string)
{
	int exit = sqlite3_open(connect_string.c_str(), &db);

	if (exit) {
		throw Error(sqlite3_errmsg(db));
	}
}

// =============== SqliteCursor =====================

SqliteCursor::SqliteCursor(sqlite3* db_)
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
	execute0(sql, data);
}

// test-only implemantation
void SqliteCursor::execute0(const dbpp::String& sql, const dbpp::InputRow& data)
{
	resultTab.clear();
	std::cout << "execute:" << sql << "\n";
	for (auto const& v : data)
		std::visit([](auto&& arg) {std::cout << arg << ", "; }, v);
	std::cout << "\n";

	char* messaggeError;
	int exit = sqlite3_exec(
		db, sql.c_str(), callback, this, &messaggeError);
	if (exit) {
		Error error(messaggeError);
		sqlite3_free(messaggeError);
		throw error;
	}
}

void SqliteCursor::execute(String const& query, InputRow const& data)
{
}
