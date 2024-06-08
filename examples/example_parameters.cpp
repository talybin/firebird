#include <iostream>
#include "firebird.hpp"

void using_parameters(fb::database db)
{
    fb::query query(db,
        "select first 3 emp_no, last_name, hire_date "
        "from employee "
        "where phone_ext > ? and job_code = ?"
    );

    // There are two ways to setup parameters for query.
    //  1. By prepare parameters before execution with query::params().
    //  2. Send parameters in query::execute().
    // These two methods can be mixed.

    // Set up one of two parameters via query::params()
    auto& p = query.params();

    // Set by index
    p[0] = 200;

    // Named parameters are not supported
    // p["PHONE_EXT"] = 201;

    // Can be read as fields
    int val = p[0];
    std::cout << "parameter " << p[0].name() << ": " << val << std::endl;

    // Send second parameter in query::execute().
    // Note, it is required to provide all parameters
    // or none in execute method. If none provided
    // parameters will previously created via params()
    // will be used.
    // Use fb::skip as placeholder for parameters
    // that already set by params() method.
    query.execute(fb::skip, "Eng");

    // Run execute again, but this time set all
    // parameters before execute
    p[1] = "Eng";
    query.execute();

    // Another way to set parameters is via set()
    query.params().set("180", "SRep");
}

int main()
{
    try {
        // Assuming Firebird server listens on port 3053
        // on localhost
        fb::database emp("localhost/3053:employee",
        { // Database connection parameters
            { isc_dpb_user_name, "sysdba" },
            { isc_dpb_password,  "masterkey" },
            { isc_dpb_lc_ctype,  "utf-8" },
        });
        emp.connect();

        using_parameters(emp);
    }
    catch (const std::exception& ex) {
        std::cout << "ERROR: " << ex.what() << std::endl;
    }
}

