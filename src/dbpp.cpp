#include <dbpp.hpp>
#include "dummy_db.hpp"
#include "sqlite_db.hpp"

using namespace dbpp;

Connection dbpp::connect(db type, std::string const& connect_string, std::string const& addParams/* = ""*/)
{
	switch (type)
	{
	//case dbpp::db::odbc:
	//	break;
	//case dbpp::db::mysql:
	//	break;
	case db::sqlite:
		return Connection(std::unique_ptr<BaseConnection>(
			new SqliteConnection(connect_string)));
	default:
		return Connection(std::unique_ptr<BaseConnection>(
			new DummyConnection));
	}
}

dbpp::Cursor::Cursor(std::shared_ptr<BaseConnection> connection_)
	: cursor(connection_->cursor())
{}

std::optional<dbpp::ResultRow> dbpp::Cursor::fetchone()
{
	return cursor->fetchone();
}

dbpp::Connection::Connection(std::shared_ptr<BaseConnection> connection_)
	: connection(connection_)
{}

Cursor dbpp::Connection::cursor()
{
	return Cursor(connection);
}

void dbpp::BaseCursor::execute(String const& sql, InputRow const&row)
{
	resultTab.clear();
	columns.clear();
	execute_impl(sql, row);
}

/// Simple implementation by reading from resutTab
/// @retval "empty" std::optional if all data recieved
std::optional<dbpp::ResultRow> dbpp::BaseCursor::fetchone()
{
	std::optional<dbpp::ResultRow> resultRow;

	if (!resultTab.empty())
	{
		resultRow = std::move(resultTab.front());
		resultTab.pop_front();
	}

	return resultRow;
}
