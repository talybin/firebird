#include "firebird.hpp"
#include <iostream>

void handle_null(fb::database db)
{
    using namespace std::literals;

    // Select 2 rows where PHONE_EXT can be null
    fb::query q(db,
        "select first 2 emp_no, phone_ext from employee where emp_no > 140");

    for (auto& row : q.execute())
    {
        // EMP_NO (smallint), not null
        fb::sqlvar emp_no = row[0];

        // We know that it can not be null, read it without check.
        std::cout << emp_no.name() << ": "
                  << emp_no.value<int>() << std::endl;

        // PHONE_EXT (varchar(4)), can be null
        fb::sqlvar phone_ext = row[1];

        // Check before read, otherwise reading value will throw on null
        if (phone_ext)
        // ...or if (!phone_ext.is_null())
            std::cout << phone_ext.name() << " (value): "
                      << phone_ext.value<std::string_view>() << std::endl;

        // Alt. use value_or() to return default value
        std::cout << phone_ext.name() << " (value_or): "
                  << phone_ext.value_or("unknown"sv) << std::endl;

        // Separate rows
        std::cout << "---" << std::endl;
    }
}

int main()
{
    try {
        fb::database emp("localhost/3053:employee");
        emp.connect();

        handle_null(emp);
    }
    catch (const std::exception& ex) {
        std::cout << "ERROR: " << ex.what() << std::endl;
    }
}

