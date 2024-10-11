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
}

void BoardStatus::processExecution()
{
    bool running = true;
    while(running)
    {
        std::string fromWinCC = waitForRequest(running);
        if(fromWinCC.size() == 0) continue;
        std::string response = executeAlfSequence(m_request.getSequence());

        updateTimePoint();
    
        auto parsedResponse = processMessageFromALF(response);

        if(m_board->type() == Board::Type::TCM){
            updateEnvironment();
        }

        WinCCResponse gbtErros = checkGBTErrors();

        Board::ParameterInfo& wordsCount = m_board->at(gbt_rate::parameters::WordsCount.data());
        Board::ParameterInfo& eventsCount  = m_board->at(gbt_rate::parameters::EventsCount.data());
        WinCCResponse gbtRates = updateRates(wordsCount.getStoredValue(), eventsCount.getStoredValue());

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
            publishAnswer(parsedResponse.response.getContents() + gbtRates.getContents() + gbtErros.getContents());
        }
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


WinCCResponse BoardStatus::checkGBTErrors()
{
    if(m_board->at(gbt_error::parameters::FifoEmpty.data()).getStoredValue() == gbt_error::constants::fifoEmpty)
    {
        return WinCCResponse();
    }

    Board::ParameterInfo& fifo = m_board->at(gbt_error::parameters::Fifo.data());
    SwtSequence gbtErrorFifoRead;
    for(uint32_t idx = 0; idx < fifo.regBlockSize; idx++)
    {
        gbtErrorFifoRead.addOperation(SwtSequence::Operation::Read, fifo.baseAddress, nullptr);
    }

    std::string alfResponse = executeAlfSequence(gbtErrorFifoRead.getSequence());

    std::array<uint32_t, gbt_error::constants::fifoSize> fifoData;
    AlfResponseParser parser(alfResponse);
    uint32_t idx = 0;
    for(auto line : parser)
    {
        fifoData[idx++] = line.frame.data;
    }
    
    std::shared_ptr<gbt_error::GBTErrorType> error =  gbt_error::parseFifoData(fifoData);
    return error->createWinCCResponse();
}


