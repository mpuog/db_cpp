#pragma once
#include "common.hpp"
#include <iostream>
using namespace dbpp;

class DummyCursor : public BaseCursor
{
public:
	//virtual void callproc(string_t const& proc_name) override
	//{		}

	DummyCursor() = default;

	void execute_impl(String const& query, InputRow const& data) override
	{
		resultTab = { { null, 21}, {"qu-qu", null} };
		for (auto const& v : data)
			std::visit([](auto&& arg) {std::cout << arg << ", "; }, v);
		std::cout << "\n";
	}
};


class DummyConnection : public BaseConnection
{
public:

	DummyConnection() = default;
	DummyConnection(DummyConnection&&) = default;

	// Inherited via BaseConnection
	virtual BaseCursor* cursor() override
	{
		return new DummyCursor;
	}

};