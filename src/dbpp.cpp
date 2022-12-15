#include "common.hpp"
#include "dummy_db.hpp"
#include "sqlite_db.hpp"

namespace dbpp
{
	Connection Connection::connect(db type, std::string const& connectString, std::string const& addParams/* = ""*/)
	{
		switch (type)
		{
			//case dbpp::db::odbc:
			//	break;
			//case dbpp::db::mysql:
			//	break;
		case db::sqlite:
			return Connection(new SqliteConnection(
				connectString, addParams));
		default:
			return Connection(new DummyConnection);
		}
	}


	// ============ Connection ========================

	Connection::~Connection() {}

	Connection::Connection(BaseConnection *connection_)
		: connection(connection_)
	{}

	Cursor Connection::cursor()
	{
		return Cursor(connection);
	}

    bool Connection::autocommit()     
    {
        return connection->autocommit();
    }
    
    void Connection::autocommit(bool autocommitFlag)
    {
        connection->autocommit(autocommitFlag);
    }

    void Connection::commit()
    {
        connection->commit();
    }


	// ============ Cursor ========================

	Cursor::~Cursor()
	{
		delete cursor;
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


};  // namespace dbpp


