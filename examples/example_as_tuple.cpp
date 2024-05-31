#include <iostream>
#include "firebird.hpp"

// EMPLOYEE table
enum class employee
{
    EMP_NO,
    FIRST_NAME,
    LAST_NAME,
    PHONE_EXT,
    HIRE_DATE,
    DEPT_NO,
    JOB_CODE,
    JOB_GRADE,
    JOB_COUNTRY,
    SALARY,
    FULL_NAME,
};

void using_tuple(fb::database db)
{
    using namespace std::literals;

    // Select 2 rows
    fb::query q(db,
        "select first 2 * from employee where emp_no > 140");

    for (auto& row : q.execute())
    {
        // Read two columns.
        // as_tuple return std::tuple<fb::sqlvar...>
        auto [emp_no, phone_ext] =
            row.as_tuple<employee::EMP_NO, employee::PHONE_EXT>();
        // ... or
        // auto [emp_no, phone_ext] = row.as_tuple<0, 3>();

        // Print EMP_NO
        std::cout << emp_no.name() << ": "
                  << emp_no.value_or(0) << std::endl;

        // Print PHONE_EXT
        std::cout << phone_ext.name() << ": "
                  << phone_ext.value_or("unknown"sv) << std::endl;

        // Read tuple without structured bindings
        auto values = row.as_tuple<employee::EMP_NO, employee::PHONE_EXT>();
        std::cout << "values: "
                  << "[ " << std::get<0>(values).value_or("none"s)
                  << ", " << std::get<1>(values).value_or("null"s)
                  << " ]" << std::endl;
    }
}

int main()
{
    try {
        fb::database emp("localhost/3053:employee");
        emp.connect();

        using_tuple(emp);
    }
    catch (const std::exception& ex) {
        std::cout << "ERROR: " << ex.what() << std::endl;
    }
}

