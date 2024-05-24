#include <iostream>
#include "firebird.hpp"

// EMPLOYEE table
class employee
{
    enum columns {
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

    fb::database db;

public:
    employee(std::string_view path)
    : db(path)
    {
        db.connect();
    }

    void handle_null()
    {
        using namespace std::literals;

        fb::query q(db,
            "select first 3 * from employee where emp_no > 140");

        for (auto& row : q.execute())
        {
            auto [emp_no, last_name, hire_date, phone_ext] =
                row.get<EMP_NO, LAST_NAME, HIRE_DATE, PHONE_EXT>();
                // or row.get<types, 0, 2, 4>()

            std::cout << emp_no.name() << ": "
                      << emp_no.value<int>() << std::endl;

            if (emp_no) {
                std::cout << emp_no.name() << ": "
                          << emp_no.value<std::string>() << std::endl;
                int val = emp_no;
                std::cout << "by operator cast: " << val << std::endl;
            }

            std::cout << phone_ext.name() << ": "
                      << phone_ext.value_or("unknown"sv) << std::endl;

            std::cout << last_name.name() << ": "
                      << last_name.value_or<std::string>("unknown") << std::endl;

            std::cout << "-------------------" << std::endl;

            #if 0
            row.visit([](auto... fields) {
                std::cout << "nr args:" << sizeof...(fields) << std::endl;
            });
            #endif
        }
    }
};

int main()
{
    try {
        employee emp("localhost/3053:employee");
        // Run query
        emp.handle_null();
    }
    catch (const std::exception& ex) {
        std::cout << "ERROR: " << ex.what() << std::endl;
    }
}

