// fbtest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "firebird.hpp"
#include "debug.hpp"

//std::ostream& operator<<(std::ostream& os, struct tm& t) { return os; }
//std::ostream& operator<<(std::ostream& os, std::chrono::microseconds& t) { return os; }


//void print_params(fb::sqlda::ptr_t&& p)
void print_params(XSQLDA* p)
{
    for (int i = 0; i < p->sqld; ++i) {
        XSQLVAR* v = &p->sqlvar[i];
        std::cout << i << ": type " << v->sqltype
                  << ", data ";
        for (int n = 0; n < 4; ++n)
            std::cout << std::hex << (int(v->sqldata[n]) & 0xff) << " ";
        std::cout << std::dec << std::endl;
    }
}


void print_result(const fb::query& q)
{
    // Description is std::vector<std::string_view>
    std::cout << "columns: [";
    for (std::string_view col : q.column_names())
        std::cout << col << ", ";
    std::cout << "]" << std::endl;

    auto cb = [](auto cno, auto cname, auto ts)
    {
        std::cout << "1: " << cno << std::endl;
        std::cout << "2: " << cname << std::endl;
        std::cout << "3: " << ts << std::endl;
        std::cout << std::endl;
    };

    #if 1
    //using tup_t = std::tuple<std::string_view, std::string_view, fb::timestamp_t>;
    //using tup_t = std::tuple<short, std::string_view, fb::timestamp_t>;
    //using tup_t = std::tuple<int, std::string_view, fb::timestamp_t>;
    using tup_t = std::tuple<std::string, std::string_view, fb::timestamp_t>;
    tup_t t;

    for (auto& row : q) {
        #if 1
        auto [cno, cname, ts] = row.get<tup_t>();
        #else
        auto [cno, cname, ts] = row.get(t);
        #endif

        std::cout << "1: " << cno << std::endl;
        std::cout << "2: " << cname << std::endl;
        std::cout << "3: " << ts << std::endl;
        std::cout << std::endl;
    }
    #else
    #if 1
    #if 1
    for (auto& row : q)
    {
        fb::timestamp_t ts = row[2];
        std::cout << "the timestamp for this row is: " << ts << std::endl;

        row.visit(cb);

        // TODO try to implement structural bindings
        //auto [v1, v2, v3] = row;
    }
    #else
    for (auto it = q.begin(); it != q.end(); ++it)
    {
        it->visit(cb);
    }
    #endif
    #else
    q.foreach([](auto cno, auto cname, auto ts)
    {
        std::cout << "1: " << cno << std::endl;
        std::cout << "2: " << cname << std::endl;
        std::cout << "3: " << ts << std::endl;
        std::cout << std::endl;

        //std::string t = ts.to_string();
    });
    #endif
    #endif
}

int main()
{
    using namespace std::literals;

#if 0
    fb::sqlda params;
    short x = 2;

    params.set("hej", 34, 2.f, 3.14, x);
    print_params(params);
    return 0;
#endif

    try {
#if 1
        fb::database db("localhost/3053:employee");
        db.connect();

        // This also prepares query
        //fb::query q(db, "select first 3 cust_no, customer from customer");

        #if 0
        fb::query q(db, "select first 3 emp_no, last_name, hire_date from employee");
        // Execute (should be able to be rerun)
        q.execute();
        #else
        fb::query q(db,
            "select first 3 emp_no, last_name, hire_date "
            "from employee "
            "where phone_ext > ? and job_code = ?"
        );

        //int x = 200;
        //auto str = "Eng"sv;

        auto& p = q.params();
        p[0] = 200;
        //p[0] = x;

        int val = p[0];
        std::cout << "got value back: " << val << std::endl;

        q.execute(fb::skip, "Eng");
        //q.execute(fb::skip, str);

        print_result(q);

        // Execute (should be able to be rerun)
        q.execute("200", "Eng");
        #endif

        //for (auto col : q.columns())
        //    std::cout << col.name() << ": " << col.size() << std::endl;

        print_result(q);

        // Try an non-select
        fb::query(db, "delete from country where country = 'test'").execute();

#endif
    }
    catch (const std::exception& ex) {
        std::cout << "ERROR: " << ex.what() << "\n";
    }

    fb::timestamp_t ts = { 0 };
    std::cout << "fb timestamp starts with: " << ts << std::endl;
    std::cout << "date should be 1989-02-06: " << fb::timestamp_t { 47563, 0 } << std::endl;

    fb::timestamp_t now = fb::timestamp_t::now();
    time_t cl = time(0);
    std::cout << "timestamp now: " << now << std::endl;
    //std::cout << "timestamp now should be: " << fb::timestamp_t::from_tm(std::localtime(&cl)) << std::endl;
    std::cout << "timestamp now should be: " << fb::timestamp_t::from_tm(std::gmtime(&cl)) << std::endl;

    std::cout << "done" << std::endl;
}
