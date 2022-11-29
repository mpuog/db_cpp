# CMakeList.txt : CMake project for db_cmake, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("db_cmake")

# Add source to this project's executable.
add_executable (db_cmake  "src/dbpp.cpp" "test/test.cpp" "src/sqlite/sqlite3.c" "src/sqlite_impl.cpp")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
set_property(TARGET db_cmake PROPERTY CXX_STANDARD 17)
#target_compile_features(INTERFACE cxx_std_17)
#add_compile_options(-pthread)


# TODO: Add tests and install targets if needed.
