#include "services/BoardStatus.h"
#include "Alfred/print.h"
#include <sstream>
#include <unistd.h>

BoardStatus::BoardStatus(std::shared_ptr<Board> board, std::list<std::string> toRefresh) : m_boardHandler(board), m_gbtFifoHandler(board)
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
    RequestExecutionResult result = executeQueuedRequests(running);
    if (result.isError) {
        Print::PrintWarning(name, "Bad request from WinCC\n" + static_cast<string>(result));
        // don't return; - the service's operation isn't disturbed by unexpected requests; START/STOP are always successful
    }
    handleSleepAndWake(1'000'000, running);
    if (!running) {
        return;
    }

    std::string response = executeAlfSequence(m_request.getSequence());

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

    auto gbtErrors = checkGbtErrors();
    if (gbtErrors.isError()) {
        publishError(gbtErrors.getContents());
    }

    Board::ParameterInfo& wordsCount = m_boardHandler.getBoard()->at(gbt::parameters::WordsCount);
    Board::ParameterInfo& eventsCount = m_boardHandler.getBoard()->at(gbt::parameters::EventsCount);
    WinCCResponse gbtRates = updateRates(wordsCount.getPhysicalValue(), eventsCount.getPhysicalValue());

    Print::PrintVerbose("Publishing board status data");
    publishAnswer(parsedResponse.response.getContents() + gbtRates.getContents() + gbtErrors.getContents());

    if (m_gbtError.get() != nullptr) {
        m_gbtError->saveErrorReport();
        m_gbtError.reset();
    }
}

void BoardStatus::updateEnvironment()
{
    m_boardHandler.getBoard()->setEnvironment(environment::parameters::SystemClock.data(),
                                              (m_boardHandler.getBoard()->at(ActualSystemClock).getPhysicalValue() == environment::constants::SourceExternalClock) ? m_boardHandler.getBoard()->getEnvironment(environment::parameters::ExtenalClock.data()) : m_boardHandler.getBoard()->getEnvironment(environment::parameters::InternalClock.data()));
    m_boardHandler.getBoard()->updateEnvironment(environment::parameters::TDC.data());
}

BoardCommunicationHandler::ParsedResponse BoardStatus::checkGbtErrors()
{
    if (m_boardHandler.getBoard()->at(gbt::parameters::FifoEmpty).getPhysicalValue() == gbt::constants::FifoEmpty) {
        return { WinCCResponse(), {} };
    }

    Board::ParameterInfo& fifo = m_boardHandler.getBoard()->at(gbt::parameters::Fifo);
    SwtSequence gbtErrorFifoRead;
    for (uint32_t idx = 0; idx < fifo.regBlockSize; idx++) {
        gbtErrorFifoRead.addOperation(SwtSequence::Operation::Read, fifo.baseAddress, nullptr);
    }

    auto fifoResponse = readFifo(m_gbtFifoHandler, fifo.name, fifo.regBlockSize);
    if (fifoResponse.isError()) {
        return { WinCCResponse(), { fifoResponse.errorReport.value() } };
    }

    std::array<uint32_t, gbt::constants::FifoSize> fifoData;
    std::copy(fifoResponse.fifoContent.front().begin(), fifoResponse.fifoContent.front().end(), fifoData.begin());

    m_gbtError = gbt::parseFifoData(fifoData);

    return { m_gbtError->createWinCCResponse(), {} };
}
