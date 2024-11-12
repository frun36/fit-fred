#include "services/BoardStatus.h"
#include "Alfred/print.h"
#include <sstream>

BoardStatus::BoardStatus(std::shared_ptr<Board> board, std::list<std::string> toRefresh) : m_boardHandler(board)
{
    std::stringstream requestStream;
    for (auto& paramName : toRefresh) {
        requestStream << paramName << ",READ\n";
    }
    std::string request = requestStream.str();
    request.pop_back();
    m_request = m_boardHandler.processMessageFromWinCC(request);
}

void BoardStatus::processExecution()
{
    bool running = true;
    // while(running)
    //{
    std::string fromWinCC = waitForRequest(running);
    if (running == false)
        return;
    std::string response = executeAlfSequence(m_request.getSequence());

    //
    m_boardHandler.getBoard()->access();
    //

    updateTimePoint();

    auto parsedResponse = m_boardHandler.processMessageFromALF(response);

    if (parsedResponse.errors.empty() == false) {
        returnError = true;
        std::stringstream error;
        for (auto& report : parsedResponse.errors) {
            error << report.what() << '\n';
        }
        error << parsedResponse.response.getContents();
        Print::PrintVerbose("Publishing error");
        publishError(error.str());
        return;
    }

    if (m_boardHandler.getBoard()->type() == Board::Type::TCM) {
        updateEnvironment();
    }

    WinCCResponse gbtErrors = checkGbtErrors();
    Board::ParameterInfo& wordsCount = m_boardHandler.getBoard()->at(gbt::parameters::WordsCount);
    Board::ParameterInfo& eventsCount = m_boardHandler.getBoard()->at(gbt::parameters::EventsCount);
    WinCCResponse gbtRates = updateRates(wordsCount.getPhysicalValue(), eventsCount.getPhysicalValue());

    //
    m_boardHandler.getBoard()->release();
    //
    
    Print::PrintVerbose("Publishing board status data");
    publishAnswer(parsedResponse.response.getContents() + gbtRates.getContents() + gbtErrors.getContents());
}

void BoardStatus::updateEnvironment()
{
    m_boardHandler.getBoard()->setEnvironment(environment::parameters::SystemClock.data(),
                            (m_boardHandler.getBoard()->at(ActualSystemClock).getPhysicalValue() == environment::constants::SourceExternalClock) ? m_boardHandler.getBoard()->getEnvironment(environment::parameters::ExtenalClock.data()) : m_boardHandler.getBoard()->getEnvironment(environment::parameters::InternalClock.data()));
    m_boardHandler.getBoard()->updateEnvironment(environment::parameters::TDC.data());
}

WinCCResponse BoardStatus::checkGbtErrors()
{
    if (m_boardHandler.getBoard()->at(gbt::parameters::FifoEmpty).getPhysicalValue() == gbt::constants::FifoEmpty) {
        return WinCCResponse();
    }

    Board::ParameterInfo& fifo = m_boardHandler.getBoard()->at(gbt::parameters::Fifo);
    SwtSequence gbtErrorFifoRead;
    for (uint32_t idx = 0; idx < fifo.regBlockSize; idx++) {
        gbtErrorFifoRead.addOperation(SwtSequence::Operation::Read, fifo.baseAddress, nullptr);
    }

    std::string alfResponse = executeAlfSequence(gbtErrorFifoRead.getSequence());

    std::array<uint32_t, gbt::constants::FifoSize> fifoData;
    AlfResponseParser parser(alfResponse);
    uint32_t idx = 0;
    for (auto line : parser) {
        if (idx >= gbt::constants::FifoSize)
            break;
        fifoData[idx++] = line.frame.data;
    }

    std::shared_ptr<gbt::GbtErrorType> error = gbt::parseFifoData(fifoData);

    return error->createWinCCResponse();
}
