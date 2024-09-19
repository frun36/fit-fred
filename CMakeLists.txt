cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 17)
project(FREDServer)

enable_testing()

add_subdirectory(core)

include_directories(mapi/include)

# Add GoogleTest using FetchContent
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/release-1.12.1.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(AlfResponseParser-Test test/AlfResponseParser/ParsingTest.cpp mapi/src/AlfResponseParser.cpp)
add_executable(SwtSequence-Test test/SwtSequence/SwtSequenceTest.cpp mapi/src/SwtSequence.cpp)
target_link_libraries(SwtSequence-Test gtest gtest_main)

add_test(NAME Unit-Test-AlfResponseParser COMMAND AlfResponseParser-Test)
add_test(NAME Unit-Test-SwtSequence COMMAND SwtSequence-Test)
