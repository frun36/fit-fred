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

# add_executable(Utils-Test test/Utils/UtilsTest.cpp)
# add_executable(AlfResponseParser-Test test/AlfResponseParser/ParsingTest.cpp mapi/src/AlfResponseParser.cpp)
# add_executable(SwtSequence-Test test/SwtSequence/SwtSequenceTest.cpp mapi/src/SwtSequence.cpp)
# add_executable(WinCCMessage-Test test/WinCCMessage/WinCCMessageTest.cpp mapi/src/WinCCRequest.cpp ${FRED})
# add_executable(Board-Test test/Board/BoardTest.cpp mapi/src/Board.cpp ${FRED})
# add_executable(Parameters-Test test/Parameters/ParametersRefactoredTest.cpp mapi/src/Parameters.cpp  mapi/src/BasicRequestHandler.cpp mapi/src/Board.cpp mapi/src/SwtSequence.cpp mapi/src/AlfResponseParser.cpp mapi/src/WinCCRequest.cpp ${FRED})
# add_executable(BasicRequestHandler-Test test/BasicRequestHandler/BasicRequestHandlerTest.cpp mapi/src/BasicRequestHandler.cpp mapi/src/Board.cpp  mapi/src/SwtSequence.cpp mapi/src/AlfResponseParser.cpp mapi/src/WinCCRequest.cpp ${FRED})
add_executable(Configurations-Test test/Configurations/ConfigurationsTest.cpp mapi/src/Configurations.cpp mapi/src/BasicRequestHandler.cpp mapi/src/Board.cpp  mapi/src/SwtSequence.cpp mapi/src/AlfResponseParser.cpp mapi/src/WinCCRequest.cpp mapi/src/Settings.cpp ${MOCKS})


# target_link_libraries(Utils-Test gtest gtest_main)
# target_link_libraries(SwtSequence-Test gtest gtest_main)
# target_link_libraries(WinCCMessage-Test Alfred clntsh occi nnz19 ociicus gtest gtest_main)
# target_link_libraries(Parameters-Test Alfred clntsh occi clntshcore nnz19 ociicus gtest gtest_main)
# target_link_libraries(Board-Test Alfred clntsh occi clntshcore nnz19 ociicus gtest gtest_main)
# target_link_libraries(BasicRequestHandler-Test Alfred clntsh occi clntshcore nnz19 ociicus gtest gtest_main gmock_main)
target_link_libraries(Configurations-Test gtest gtest_main gmock_main)

# add_test(NAME Unit-Test-Utils COMMAND Utils-Test)
# add_test(NAME Unit-Test-AlfResponseParser COMMAND AlfResponseParser-Test)
# add_test(NAME Unit-Test-SwtSequence COMMAND SwtSequence-Test)
# add_test(NAME Unit-Test-WinCCMessage COMMAND WinCCMessage-Test)
# add_test(NAME Unit-Test-Board COMMAND Board-Test)
# add_test(NAME Unit-Test-Parameters COMMAND Parameters-Test)
# add_test(NAME Unit-Test-BasicRequestHandler COMMAND BasicRequestHandler-Test)
add_test(NAME Unit-Test-Configurations COMMAND Configurations-Test)
