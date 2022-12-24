#include "common.hpp"
#include "sqlite_db.hpp"

#ifdef DBPP_ODBC
#include "odbc_db.hpp"
#endif // DBPP_ODBC

namespace dbpp
{
	Connection Connection::connect(db type, std::string const& connectString, std::string const& addParams/* = ""*/)
	{
		switch (type)
		{
#ifdef DBPP_ODBC
		case dbpp::db::odbc:
			return Connection(new OdbcConnection(connectString));
#endif // DBPP_ODBC
		case db::sqlite:
			return Connection(new SqliteConnection(
				connectString, addParams));
		//case dbpp::db::mysql:
		//	break;  
		default:
			throw Error("Unknown db type");
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

    void Connection::rollback()
    {
        connection->rollback();
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
		rowcount_ = cursor->execute(query, data, columnsInfo);
	}

	void Cursor::executemany(String const& query, 
		InputTab const& input_data)
	{
		rowcount_ = cursor->executemany(query, input_data);
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

	ResultTab Cursor::fetchmany(int size)
	{
		if (-1 == size)
			size = arraysize_;

		ResultTab rt;
		for (int i = 0; i < size; ++i)
		{
			auto row = fetchone();
			if (!row)
				break;
			rt.push_back(std::move(*row));
		}
		return rt;
	}


};  // namespace dbpp


