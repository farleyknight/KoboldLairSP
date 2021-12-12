# CONAN

To use `conan` in this project, you must run the following commands:

* `conan install .`
  - This should install all of the dependencies defined in `conanfile.py`
* `conan build .`
  - This will attempt to compile your source code.
* `conan export-pkg .`
  - This will install the package locally.
  
  

## TESTING

In order to verify that a `conan` package works, we have an additional directory, called `test_package`. In that directory is a test package that will use the library you've created in the main project directory. 

```bash
$ cd test_package
$ conan install .
$ conan build .
$ ./bin/run_tests
$ ./bin/example
```
