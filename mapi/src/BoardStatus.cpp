#include"BoardStatus.h"
#include"GBT.h"
#include<sstream>

BoardStatus::BoardStatus(std::shared_ptr<Board> board, std::list<std::string> toRefresh):
BasicRequestHandler(board)
{
    std::stringstream requestStream;
    for(auto& paramName: toRefresh)
    {
        requestStream << paramName << ",READ\n";
    }
    std::string request = requestStream.str();
    request.pop_back();
    m_request = processMessageFromWinCC(request);

    Board::ParameterInfo& fifo = m_board->at(GBT_ERROR_REPORT_FIT0);
    for(uint32_t idx = 0; idx < fifo.regBlockSize; idx++)
    {
        m_gbtErrorFifoRead.addOperation(SwtSequence::Operation::Read, fifo.baseAddress, nullptr);
    }
}

void BoardStatus::processExecution()
{
    bool running = true;
    while(running)
    {
        std::string fromWinCC = waitForRequest(running);
        if(fromWinCC.size() == 0) continue;
        std::string response = executeAlfSequence(m_request.getSequence());

        m_currTimePoint = std::chrono::steady_clock::now();
        m_pomTimeInterval = std::chrono::duration_cast<std::chrono::milliseconds>(m_currTimePoint-m_lastTimePoint);
    
        auto parsedResponse = processMessageFromALF(response);

        if(m_board->type() == Board::Type::TCM){
            updateEnvironment();
        }
        checkGBTErrorReport(parsedResponse.response);
        calculateGBTRate(parsedResponse.response);

        if(parsedResponse.errors.size() != 0){
            returnError = true;
            std::stringstream error;
            for(auto& report: parsedResponse.errors)
            {
                error << report.what() << '\n';
            }
            error << parsedResponse.response.getContents();
            publishError(error.str());
        }
        else{
            publishAnswer(parsedResponse.response.getContents());
        }
        m_lastTimePoint = m_currTimePoint;
    }
}

void BoardStatus::updateEnvironment()
{
    m_board->setEnvironment(SYSTEM_CLOCK_VNAME, (m_board->at(ACTUAL_SYSTEM_CLOCK_NAME).getStoredValue() == EXTERNAL_CLOCK) ?
                m_board->getEnvironment(EXTERNAL_CLOCK_VNAME):
                m_board->getEnvironment(INTERNAL_CLOCL_VNAME)
                );
    m_board->updateEnvironment(TDC_VNAME);
}

void BoardStatus::calculateGBTRate(WinCCResponse& response)
{
    double frequency = 1000.0/ static_cast<double>(m_pomTimeInterval.count());
    double wordsRate = (m_board->at(WORDS_COUNT_NAME).getStoredValue() - m_gbtRate.wordsCount) * frequency;
    double eventsRate =  (m_board->at(EVENTS_COUNT_NAME).getStoredValue() - m_gbtRate.eventsCount) * frequency;
    m_gbtRate.wordsCount = m_board->at(WORDS_COUNT_NAME).getStoredValue();
    m_gbtRate.eventsCount = m_board->at(EVENTS_COUNT_NAME).getStoredValue();
    response.addParameter(GBT_WORD_RATE_NAME, {wordsRate});
    response.addParameter(GBT_EVENT_RATE_NAME, {eventsRate});
}

void BoardStatus::checkGBTErrorReport(WinCCResponse& response)
{
    if(m_board->at(GBT_ERROR_REPORT_EMPTY).getStoredValue() == 1){
       return;
    }
    else{
        response.addParameter(GBT_ERROR_NAME, {1});
        readGBTErrorFIFO(response);
    }
}

void BoardStatus::readGBTErrorFIFO(WinCCResponse& response)
{
    std::string alfResponse = executeAlfSequence(m_gbtErrorFifoRead.getSequence());
    AlfResponseParser pareser(alfResponse);

    uint32_t idx = 0;
    GBTErrorReport report;

    for(auto line: pareser)
    {
        report.buffer[idx++] = line.frame.data;
    }
}
