cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 17)
project(FREDServer)

enable_testing()

add_subdirectory(core)

include_directories(mapi/include)
add_executable(AlfResponseParser-Test test/AlfResponseParser/ParsingTest.cpp mapi/src/AlfResponseParser.cpp)
add_test(NAME Unit-Test-AlfResponseParser COMMAND AlfResponseParser-Test)
