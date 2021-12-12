cmake_minimum_required(VERSION 3.1)
project(KoboldLairSP-PackageTest CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

find_package("fmt")

add_executable(example example.cpp)

target_compile_features(example PUBLIC cxx_std_17)
target_include_directories(example PRIVATE ${CONAN_INCLUDE_DIRS})
target_link_libraries(example
  ${CONAN_LIBS})

add_executable(run_tests run_tests.cpp)

target_compile_features(run_tests PUBLIC cxx_std_17)
target_include_directories(run_tests PRIVATE ${CONAN_INCLUDE_DIRS})
target_link_libraries(run_tests
  ${CONAN_LIBS})
