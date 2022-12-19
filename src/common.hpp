#pragma once
#include <dbpp.hpp>

namespace dbpp
{
	class BaseConnection
	{
	protected:
		BaseConnection(BaseConnection&&) = default;
		BaseConnection() = default;
	public:
		virtual ~BaseConnection() {}
		virtual BaseCursor *cursor() = 0;
        virtual bool autocommit() {
            return true;
        }
        virtual void autocommit(bool autocommitFlag) 
        {}
        virtual void commit() 
        {}
        virtual void rollback() 
        {}
	};

	class BaseCursor
	{
	protected:
		BaseCursor() = default;

		virtual void execute_impl(
			String const& sql, InputRow const&) = 0;
	public:
        /// Data fo stupid method when select operator puts all data to resultTab.
        /// Slow if select without restrictions on quantity, but not all data need later.
		std::deque<ResultRow> resultTab;
        /// Columns names
		std::vector<String> columns;
		virtual 		~BaseCursor() {}
		BaseCursor(BaseCursor&&) = default;

		void execute(String const& sql, InputRow const &row)
		{
			resultTab.clear();
			columns.clear();
			execute_impl(sql, row);
		}

		/// Simple universal implementation
		/// @todo db specific implementation
		virtual void executemany(String const& sql,
			InputTab const& input_data)
		{
			resultTab.clear();
			columns.clear();
			for (auto const& row : input_data)
			{
				execute_impl(sql, row);
			}
		}


		/// Simple implementation by reading from resutTab
		/// @retval "empty" std::optional if all data recieved
		virtual std::optional<ResultRow> fetchone()
		{
			std::optional<ResultRow> resultRow;

			if (!resultTab.empty())
			{
				resultRow = std::move(resultTab.front());
				resultTab.pop_front();
			}

			return resultRow;
		}
	};

}  // namespace dbpp


