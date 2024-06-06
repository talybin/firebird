# Firebird C++ Library
Header only C++ wrapper on Firebird database client C API. Require C++17.

## Sneak Peek
```cpp
#include <firebird.hpp>
#include <iostream>

int main() {
    fb::database emp("employee");
    emp.connect();
    // Using default transaction from fb::database
    fb::query query(emp, "select last_name from employee");
    // There are many ways to read result from the query
    for (auto& row : query.execute()) {
        std::cout << row["LAST_NAME"].value_or("") << std::endl;
    }
    // Database will disconnect when goes out of scope here
}
```
```cpp
#include <firebird.hpp>
#include <iostream>

int main() {
    fb::database emp("employee");
    emp.connect();
    // Database has a default transaction, but you can create a new one
    fb::transaction trans(emp);

    try {
        fb::query query(trans,
            "insert into project (proj_id, proj_name, proj_desc) "
            "values ('TEST', ?, ?)");
        // One way to set parameters is to send in execute() call
        // (proj_desc is a blob type here)
        query.execute("New project", "This project using Firebird C++ library");
        trans.commit();
    }
    catch (const fb::exception& ex) {
        std::cout << "ERROR: " << ex.what() << std::endl;
        trans.rollback();
    }
}
```
More examples are given in the examples directory.

## Documentation
Documentation can be built with doxygen. Under Linux run
```sh
make docs
```

## Features
* Creates a view on parameters and result from the query. No unnecessary copying and allocations.
* Has expandable built-in type converter making it easy to read and write fields and parameters.
* Has visitor pattern for received data.
  ```cpp
  query.foreach([](auto... fields) { });
  ```
* Methods to convert to and from `std::time_t` and `std::tm` for SQL timestamp.
* Fully supporting BLOB type.
* Possibility to create new database from the code.
* Has support for `execute_immediate`.

## Creating a single header
You can grab single header from single directory. There also a script called `make_single.py`
provided in the repository. To create single header locally, run:
```sh
make single
```

## Testing
There are not many tests available, but look in `tests` directory. These can be compiled with:
```sh
make tests
```

## Adding library to your project
### Linux
1. Install firebird dev package
```sh
sudo apt install firebird-dev
```
2. Include `firebird.hpp` in your program and link with `fbclient` library
```sh
g++ -Ifirebird/include your_app.cpp -lfbclient
```
### Windows
1. Download and extract zip package of Firebird from https://firebirdsql.org/en/server-packages/.
2. You will need to copy following files to your project's directory:
   * include\ibase.h
   * include\iberror.h
   * include\ib_util.h
   * lib\fbclient_ms.lib
   * lib\ib_util_ms.lib
   * fbclient.dll
3. Link both lib files to your project. From "Project" menu choose "Properties". Expand "Linker" and choose "Input".
   Add `fbclient_ms.lib` and `ib_util_ms.lib` to "Additional Dependencies".
4. Include `firebird.hpp` in your program and compile.

