# CMakeList.txt : CMake project for db_cmake, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("db_cmake")

# Add source to this project's executable.
IF (NOT WIN32)
    add_executable (db_cmake "test/test.cpp"
    "src/dbpp.cpp" "src/sqlite_db.cpp"
    "src/sqlite/sqlite3.c" )
ELSE()
    add_executable (db_cmake  "test/test.cpp"
    "src/dbpp.cpp" "src/odbc_db.cpp" "src/sqlite_db.cpp"
     "src/sqlite/sqlite3.c")
ENDIF()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
set_property(TARGET db_cmake PROPERTY CXX_STANDARD 17)
IF (NOT WIN32)
	target_link_libraries(db_cmake dl pthread)
ENDIF()

# TODO: Add tests and install targets if needed.
