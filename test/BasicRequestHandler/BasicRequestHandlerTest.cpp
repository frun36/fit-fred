// Include necessary headers
#include <gtest/gtest.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include <cstdint>
#include <vector>
#include <random>

#include"BasicRequestHandler.h"
#include"Board.h"

std::shared_ptr<Board> createTestBoard()
{
    std::shared_ptr<Board> board = std::make_shared<Board>("TestBoard", 0x0);
    {
        Board::ParameterInfo::Equation rawToPhysic = { "BoardTemperature*0.1", { "BoardTemperature" } };
        Board::ParameterInfo::Equation physicToRaw = { "BoardTemperature/0.1", { "BoardTemperature" } };

        Board::ParameterInfo param(
            "BoardTemperature",
            0x5,
            0,
            16,
            1,
            Board::ParameterInfo::ValueEncoding::Signed,
            0.0,
            65535.0,
            rawToPhysic,
            physicToRaw,
            false,
            true
        );

    
        board->emplace(std::move(param));
    }
    {
        Board::ParameterInfo param(
            "ActualClockSource",
            0x0F,
            3,
            1,
            1,
            Board::ParameterInfo::ValueEncoding::Unsigned,
            0,
            1,
            Board::ParameterInfo::Equation::Empty(),
            Board::ParameterInfo::Equation::Empty(),
            false,
            false
        );

        board->emplace(std::move(param));
    }
    {
        Board::ParameterInfo param(
            "GBTRxReady",
            0x0F,
            4,
            1,
            1,
            Board::ParameterInfo::ValueEncoding::Unsigned,
            0,
            1,
            Board::ParameterInfo::Equation::Empty(),
            Board::ParameterInfo::Equation::Empty(),
            false,
            false
        );

        board->emplace(std::move(param));
    }
    return board;
}

TEST(BasicRequestHandlerTest, ProcessWinCCMessageReadTest)
{
    std::shared_ptr<Board> testBoard = createTestBoard();
    BasicRequestHandler handler(testBoard);
    SwtSequence created = handler.processMessageFromWinCC("BoardTemperature,READ\nActualClockSource,READ\nGBTRxReady,READ");

    SwtSequence expected;
    expected.addOperation(SwtSequence::Operation::Read, 0x0F, nullptr, true);
    expected.addOperation(SwtSequence::Operation::Read, 0x05, nullptr, true);
    EXPECT_EQ(created.getSequence(),expected.getSequence());
}

TEST(BasicRequestHandlerTest, ProcessWinCCMessageWriteTest)
{
    std::shared_ptr<Board> testBoard = createTestBoard();
    BasicRequestHandler handler(testBoard);
    SwtSequence created = handler.processMessageFromWinCC("ActualClockSource,WRITE,1\nGBTRxReady,WRITE,1");

    SwtSequence expected;
    std::array<uint32_t, 2> data = {static_cast<uint32_t>(~(0x18)),0x18};
    expected.addOperation(SwtSequence::Operation::RMWbits, 0x0F, data.data(), false);
    expected.addOperation(SwtSequence::Operation::Read, 0x0F, nullptr, true);
    EXPECT_EQ(created.getSequence(),expected.getSequence());
}

TEST(BasicRequestHandlerTest, ProcessWinCCMessageWriteResponseSuccess)
{
    std::shared_ptr<Board> testBoard = createTestBoard();
    BasicRequestHandler handler(testBoard);
    SwtSequence created = handler.processMessageFromWinCC("ActualClockSource,WRITE,1\nGBTRxReady,WRITE,1");

    SwtSequence expected;
    std::array<uint32_t, 2> data = {static_cast<uint32_t>(~(0x18)),0x18};
    expected.addOperation(SwtSequence::Operation::RMWbits, 0x0F, data.data(), false);
    expected.addOperation(SwtSequence::Operation::Read, 0x0F, nullptr, true);
    EXPECT_EQ(created.getSequence(),expected.getSequence());

    std::string alfResponse("success\n0\n0x0000000000F00000018");
    auto parsed = handler.processMessageFromALF(alfResponse);

    EXPECT_EQ(parsed.second.size(),0) << parsed.second.front().what() << std::endl;
    EXPECT_EQ(parsed.first.getContents(),"ActualClockSource,1\nGBTRxReady,1\n");
}

TEST(BasicRequestHandlerTest, ProcessWinCCMessageWriteResponseFailure)
{
    std::shared_ptr<Board> testBoard = createTestBoard();
    BasicRequestHandler handler(testBoard);
    SwtSequence created = handler.processMessageFromWinCC("ActualClockSource,WRITE,1\nGBTRxReady,WRITE,1");

    SwtSequence expected;
    std::array<uint32_t, 2> data = {static_cast<uint32_t>(~(0x18)),0x18};
    expected.addOperation(SwtSequence::Operation::RMWbits, 0x0F, data.data(), false);
    expected.addOperation(SwtSequence::Operation::Read, 0x0F, nullptr, true);
    EXPECT_EQ(created.getSequence(),expected.getSequence());

    std::string alfResponse("success\n0\n0x0000000000F00000028");
    auto parsed = handler.processMessageFromALF(alfResponse);

    EXPECT_EQ(parsed.second.size(),1);
    EXPECT_EQ(parsed.second.front().what(),"ERROR - GBTRxReady: WRITE FAILED: Received 0.000000, Expected 1.000000");
    EXPECT_EQ(parsed.first.getContents(),"ActualClockSource,1\n");
}