cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # Enforce the C++20 standard

project(FIT-FREDServer)

enable_testing()

add_definitions(-DFIT_UNIT_TEST)
add_compile_options(-g -Wall -Wextra)

add_compile_options(-g -Wall -Wextra)

include_directories(
    mapi/include 
    core/include 
    core/include/FREDServer 
    core/include/occi
)
link_directories(core/lib)

# Add GoogleTest using FetchContent
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

file(GLOB MOCKS
    "test/mocks/src/databaseinterface.cpp" 
    "test/mocks/src/mapi.cpp"
    "test/mocks/src/utility.cpp"
)

macro(create_test TEST_NAME)
    add_executable(${TEST_NAME}-Test ${ARGN})
    target_link_libraries(${TEST_NAME}-Test gtest gtest_main gmock_main)
    add_test(NAME Unit-Test-${TEST_NAME} COMMAND ${TEST_NAME}-Test)
endmacro()

create_test(Utils 
    test/Utils/UtilsTest.cpp
)

create_test(AlfResponseParser
    test/AlfResponseParser/ParsingTest.cpp 
    mapi/src/AlfResponseParser.cpp
)

create_test(SwtSequence
    test/SwtSequence/SwtSequenceTest.cpp 
    mapi/src/SwtSequence.cpp
)

create_test(WinCCMessage
    test/WinCCMessage/WinCCMessageTest.cpp 
    mapi/src/WinCCRequest.cpp 
    ${MOCKS}
)

# create_test(Board
#     test/Board/BoardTest.cpp
#     mapi/src/Board.cpp 
#     ${MOCKS}
# )

# create_test(Parameters
#     test/Parameters/ParametersRefactoredTest.cpp 
#     mapi/src/Parameters.cpp  
#     mapi/src/BasicRequestHandler.cpp 
#     mapi/src/Board.cpp 
#     mapi/src/SwtSequence.cpp 
#     mapi/src/AlfResponseParser.cpp 
#     mapi/src/WinCCRequest.cpp 
#     ${MOCKS}
# )

# create_test(BasicRequestHandler
#     test/BasicRequestHandler/BasicRequestHandlerTest.cpp 
#     mapi/src/BasicRequestHandler.cpp 
#     mapi/src/Board.cpp  
#     mapi/src/SwtSequence.cpp 
#     mapi/src/AlfResponseParser.cpp 
#     mapi/src/WinCCRequest.cpp 
#     ${MOCKS}
# )

create_test(
    Configurations
    test/Configurations/ConfigurationsTest.cpp 
    mapi/src/Configurations.cpp 
    mapi/src/BasicRequestHandler.cpp 
    mapi/src/Board.cpp  
    mapi/src/SwtSequence.cpp 
    mapi/src/AlfResponseParser.cpp 
    mapi/src/WinCCRequest.cpp 
    mapi/src/Environment.cpp 
    ${MOCKS}
)

