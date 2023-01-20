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
		virtual bool autocommit() = 0;
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

		/// Data for stupid method when select operator puts all data to resultTab.
		/// May be slow if select without restrictions on quantity, but not all data need later.
		std::deque<ResultRow> resultTab;

		virtual int execute_impl(
			String const& sql, InputRow const&row,
			std::deque<ResultRow> &resultTab, ColumnsInfo& columnsInfo) = 0;
	public:

		virtual 		~BaseCursor() {}
		BaseCursor(BaseCursor&&) = default;

		int execute(String const& sql, InputRow const &row, 
			ColumnsInfo &columnsInfo)
		{
			resultTab.clear();
			columnsInfo.clear();
			int execute_return = execute_impl(sql, row, resultTab, columnsInfo);
			return resultTab.empty() ? execute_return : (int)resultTab.size();
		}

		/// Simple universal implementation
		/// @todo db specific implementation with one time sql statement init
		virtual int executemany(String const& sql, 
			InputTab const& input_data)
		{
			resultTab.clear();
			ColumnsInfo columnsInfo;
			int count = 0;
			for (auto const& row : input_data)
			{
				count += execute_impl(sql, row, resultTab, columnsInfo);
			}
			return count;
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


