cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # Enforce the C++20 standard

project(FIT-FREDServer)

enable_testing()

option(test-only "Build only the tests" OFF)
option(disable-test "Disable tests" OFF)
if (NOT test-only)
    add_subdirectory(core)
endif()

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

file(GLOB FRED 
    "core/src/Parser/utility.cpp"
    "core/src/Parser/parser.cpp"
    "core/src/Parser/processmessage.cpp"
    "core/src/Fred/alfclients.cpp"
    "core/src/Fred/alfrpcinfo.cpp"
    "core/src/Fred/fred.cpp"
    "core/src/Fred/fredtopics.cpp"
    "core/src/Fred/mappedcommand.cpp"
    "core/src/Fred/groupcommand.cpp"
    "core/src/Fred/queue.cpp"
    "core/src/Fred/queueexecutor.cpp"
    "core/src/Fred/section.cpp"
    "core/src/Fred/selectcommand.cpp"
    "core/src/Fred/alfinfo.cpp"
    "core/src/Fred/crualfrpcinfo.cpp"
    "core/src/Fred/cruregistercommand.cpp"
    "core/src/Fred/Config/groups.cpp"
    "core/src/Fred/Config/instructions.cpp"
    "core/src/Fred/Config/mapping.cpp"
    "core/src/Fred/Config/crumapping.cpp"
    "core/src/Fred/Config/llamapping.cpp"
    "core/src/Fred/llalock.cpp"
    "core/src/Fred/llaalfrpcinfo.cpp"
    "core/src/Fred/Mapi/mapi.cpp"
    "core/src/Fred/Mapi/iterativemapi.cpp"
    "core/src/Fred/Mapi/mapigroup.cpp"
    "core/src/Fred/Mapi/indefinitemapi.cpp"
    "core/src/Fred/Protocols/SCA.cpp"
    "core/src/Fred/Protocols/SWT.cpp"
    "core/src/Fred/Protocols/IC.cpp"
    "core/src/Fred/Protocols/CRORC.cpp"
    "core/src/Fred/Protocols/SCAMFT.cpp"
    "core/src/Fred/Protocols/CRU.cpp"
    "core/src/Fred/Database/databaseinterface.cpp"
    "core/src/Fred/Database/multitype.cpp"
)

if (NOT disable-test)

    add_executable(Utils-Test test/Utils/UtilsTest.cpp)
    add_executable(AlfResponseParser-Test test/AlfResponseParser/ParsingTest.cpp mapi/src/AlfResponseParser.cpp)
    add_executable(SwtSequence-Test test/SwtSequence/SwtSequenceTest.cpp mapi/src/SwtSequence.cpp)
    add_executable(WinCCMessage-Test test/WinCCMessage/WinCCMessageTest.cpp mapi/src/WinCCRequest.cpp ${FRED})
    add_executable(Board-Test test/Board/BoardTest.cpp mapi/src/Board.cpp ${FRED})
    add_executable(Parameters-Test test/Parameters/ParametersRefactoredTest.cpp mapi/src/Parameters.cpp  mapi/src/BasicRequestHandler.cpp mapi/src/Board.cpp mapi/src/SwtSequence.cpp mapi/src/AlfResponseParser.cpp mapi/src/WinCCRequest.cpp ${FRED})
    add_executable(BasicRequestHandler-Test test/BasicRequestHandler/BasicRequestHandlerTest.cpp mapi/src/BasicRequestHandler.cpp mapi/src/Board.cpp  mapi/src/SwtSequence.cpp mapi/src/AlfResponseParser.cpp mapi/src/WinCCRequest.cpp ${FRED})


    target_link_libraries(Utils-Test gtest gtest_main)
    target_link_libraries(SwtSequence-Test gtest gtest_main)
    target_link_libraries(WinCCMessage-Test Alfred clntsh occi nnz19 ociicus gtest gtest_main)
    target_link_libraries(Parameters-Test Alfred clntsh occi clntshcore nnz19 ociicus gtest gtest_main)
    target_link_libraries(Board-Test Alfred clntsh occi clntshcore nnz19 ociicus gtest gtest_main)
    target_link_libraries(BasicRequestHandler-Test Alfred clntsh occi clntshcore nnz19 ociicus gtest gtest_main gmock_main)

    add_test(NAME Unit-Test-Utils COMMAND Utils-Test)
    add_test(NAME Unit-Test-AlfResponseParser COMMAND AlfResponseParser-Test)
    add_test(NAME Unit-Test-SwtSequence COMMAND SwtSequence-Test)
    add_test(NAME Unit-Test-WinCCMessage COMMAND WinCCMessage-Test)
    add_test(NAME Unit-Test-Board COMMAND Board-Test)
    add_test(NAME Unit-Test-Parameters COMMAND Parameters-Test)
    add_test(NAME Unit-Test-BasicRequestHandler COMMAND BasicRequestHandler-Test)

endif()