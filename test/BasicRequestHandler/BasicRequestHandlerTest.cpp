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

std::shared_ptr<Board> createTestBoard()
{
    std::shared_ptr<Board> board = std::make_shared<Board>("TestBoard", 0x1000);
    Board::ParameterInfo::Equation rawToPhysic = { "ParamUnsigned", { "ParamUnsigned" } };
    Board::ParameterInfo::Equation physicToRaw = { "ParamUnsigned", { "ParamUnsigned" } };

    Board::ParameterInfo param(
        "ParamUnsigned",
        0x1000,
        0,
        16,
        1,
        Board::ParameterInfo::ValueEncoding::Unsigned,
        0.0,
        65535.0,
        rawToPhysic,
        physicToRaw,
        false,
        false
    );

    board->emplace(param);
    return board;
}

TEST(BasicRequestHandlerTest, ProcessWinCCMessageTest)
{
    std::shared_ptr<Board> testBoard = createTestBoard();
    BasicRequestHandler handler(testBoard);
    
    
}