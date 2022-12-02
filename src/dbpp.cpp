#include "common.hpp"
#include "dummy_db.hpp"
#include "sqlite_db.hpp"

namespace dbpp
{
	Connection connect(db type, std::string const& connect_string, std::string const& addParams/* = ""*/)
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

	Cursor::Cursor(std::shared_ptr<BaseConnection> connection_)
		: cursor(connection_->cursor())
	{}

	void Cursor::execute(String const& query, InputRow const& data)
	{
		cursor->execute(query, data);
	}

	void Cursor::executemany(String const& query, 
		InputTab const& input_data)
	{
		for (auto const& row : input_data)
		{
			cursor->execute(query, row);
		}
	}

	std::optional<ResultRow> Cursor::fetchone()
	{
		return cursor->fetchone();
	}

	ResultTab Cursor::fetchall() //< @todo fetchmany();
	{
		ResultTab rt;
		while (auto row = fetchone())
			rt.push_back(std::move(*row));
		return rt;
	}

	Connection::Connection(std::shared_ptr<BaseConnection> connection_)
		: connection(connection_)
	{}

	Cursor Connection::cursor()
	{
		return Cursor(connection);
	}

	void BaseCursor::execute(String const& sql, InputRow const& row)
	{
		resultTab.clear();
		columns.clear();
		execute_impl(sql, row);
	}

	/// Simple implementation by reading from resutTab
	/// @retval "empty" std::optional if all data recieved
	std::optional<ResultRow> BaseCursor::fetchone()
	{
		std::optional<ResultRow> resultRow;

		if (!resultTab.empty())
		{
			resultRow = std::move(resultTab.front());
			resultTab.pop_front();
		}

		return resultRow;
	}

};  // namespace dbpp


