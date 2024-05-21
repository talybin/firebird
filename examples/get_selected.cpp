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

    using types = std::tuple<
        int,                // EMP_NO
        std::string_view,   // FIRST_NAME
        std::string_view,   // LAST_NAME
        std::string_view,   // PHONE_EXT
        fb::timestamp_t,    // HIRE_DATE
        std::string_view,   // DEPT_NO
        std::string_view,   // JOB_CODE
        int,                // JOB_GRADE
        std::string_view,   // JOB_COUNTRY
        size_t,             // SALARY
        std::string_view    // FULL_NAME
    >;

    fb::database db;

public:
    employee(std::string_view path)
    : db(path)
    {
        db.connect();
    }

    void select_some()
    {
        fb::query q(db,
            //"select first 3 emp_no, last_name, hire_date from employee");
            "select first 3 * from employee");

        //using some_t = fb::select<types, EMP_NO, LAST_NAME, HIRE_DATE>;

        for (auto& row : q.execute())
        {
            auto [emp_no, last_name, hire_date] =
                row.select<types, EMP_NO, LAST_NAME, HIRE_DATE>();
                // or row.select<types, 0, 2, 4>()

            std::cout
                << "["
                << emp_no << ", "
                << last_name << ", "
                << hire_date
                << "]" << std::endl;

            // Get all fields and print some of these
            auto values = row.get<types>();
            std::cout << "salary for "
                      << std::get<FULL_NAME>(values)
                      << " is "
                      << std::get<SALARY>(values)
                      << std::endl;
        }
    }
};

int main()
{
    try {
        employee emp("localhost/3053:employee");
        // Run query
        emp.select_some();
    }
    catch (const std::exception& ex) {
        std::cout << "ERROR: " << ex.what() << std::endl;
    }
}

