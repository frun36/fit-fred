#include <gtest/gtest.h>
#include <string>
#include <memory>

#include "Parameters.h"
#include "Board.h"
#include "SwtSequence.h"
#include "WinCCRequest.h"
#include "WinCCResponse.h"

// Function to create a test board
std::shared_ptr<Board> createTestBoard()
{
    std::shared_ptr<Board> board = std::make_shared<Board>("TestBoard", 0x0);
    {
        Board::ParameterInfo::Equation electronicToPhysic = { "BoardTemperature*0.1", { "BoardTemperature" } };
        Board::ParameterInfo::Equation physicToElectronic = { "BoardTemperature/0.1", { "BoardTemperature" } };

        Board::ParameterInfo param(
            "BoardTemperature",
            0x5,
            0,
            16,
            1,
            Board::ParameterInfo::ValueEncoding::Signed,
            0.0,
            65535.0,
            electronicToPhysic,
            physicToElectronic,
            false,
            true);

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
            false);

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
            false);

        board->emplace(std::move(param));
    }
    return board;
}

TEST(ParametersTest, ProcessInputMessage_ReadCommands)
{
    // Arrange
    auto testBoard = createTestBoard();
    Parameters parameters(testBoard);

    std::string inputMessage = "BoardTemperature,READ\nActualClockSource,READ\nGBTRxReady,READ";

    // Act
    std::string output;
    EXPECT_NO_THROW({
        output = parameters.processInputMessage(inputMessage);
    });

    // Assert
    EXPECT_FALSE(output.empty());
    // Additional checks can be added if the expected output is known
}

TEST(ParametersTest, ProcessInputMessage_WriteCommands)
{
    // Arrange
    auto testBoard = createTestBoard();
    Parameters parameters(testBoard);

    std::string inputMessage = "ActualClockSource,WRITE,1\nGBTRxReady,WRITE,1";

    // Act
    std::string output;
    EXPECT_NO_THROW({
        output = parameters.processInputMessage(inputMessage);
    });

    // Assert
    EXPECT_FALSE(output.empty());
}

TEST(ParametersTest, ProcessInputMessage_InvalidCommand)
{
    // Arrange
    auto testBoard = createTestBoard();
    Parameters parameters(testBoard);

    std::string inputMessage = "InvalidParameter,READ";

    // Act & Assert
    EXPECT_THROW({ parameters.processInputMessage(inputMessage); }, std::exception);
}

TEST(ParametersTest, ProcessOutputMessage_SuccessfulResponse)
{
    // Arrange
    auto testBoard = createTestBoard();
    Parameters parameters(testBoard);

    std::string inputMessage = "ActualClockSource,READ\nGBTRxReady,READ";
    parameters.processInputMessage(inputMessage);

    std::string alfResponse = "success\n0\n0x0000000000F00000018";

    // Act
    std::string output = parameters.processOutputMessage(alfResponse);

    // Assert
    EXPECT_FALSE(parameters.returnError);
    EXPECT_FALSE(output.empty());
    // Optionally, check the output content
}

TEST(ParametersTest, ProcessOutputMessage_ErrorResponse)
{
    // Arrange
    auto testBoard = createTestBoard();
    Parameters parameters(testBoard);

    std::string inputMessage = "ActualClockSource,WRITE,1\nGBTRxReady,WRITE,1";
    parameters.processInputMessage(inputMessage);

    std::string alfResponse = "success\n0\n0x0000000000F00000028";

    // Act
    std::string output = parameters.processOutputMessage(alfResponse);

    // Assert
    EXPECT_TRUE(parameters.returnError);
    EXPECT_FALSE(output.empty());
    EXPECT_NE(output.find("ERROR - GBTRxReady: WRITE FAILED"), std::string::npos);
}

TEST(ParametersTest, ProcessInputMessage_ExceptionHandling)
{
    // Arrange
    auto testBoard = createTestBoard();
    Parameters parameters(testBoard);

    std::string inputMessage = "BoardTemperature,WRITE"; // Missing value

    // Act & Assert
    EXPECT_THROW({ parameters.processInputMessage(inputMessage); }, std::exception);

    EXPECT_FALSE(parameters.returnError);
}
