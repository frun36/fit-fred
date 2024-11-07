#include "gbtInterfaceUtils.h"
#include "Alfred/print.h"
#include <cstring>
#include<fstream>
#include<ctime>
#include"utils.h"

uint32_t build32(const uint8_t* buffer, size_t& idx){
    uint32_t word = static_cast<uint32_t>(static_cast<uint32_t>(idx) << 23u) +
            static_cast<uint32_t>(static_cast<uint32_t>(idx+1) << 15u) +
            static_cast<uint32_t>(static_cast<uint32_t>(idx+2) << 7u) +
            static_cast<uint32_t>(idx+3);
    idx += 4;
    return word;
}

uint16_t build16(const uint8_t* buffer, size_t& idx){
    uint16_t word = static_cast<uint32_t>(static_cast<uint32_t>(idx) << 7u) + static_cast<uint32_t>(idx+1);
    idx += 2;
    return word;
}


namespace gbt
{


[[nodiscard]] std::shared_ptr<GbtErrorType> parseFifoData(const std::array<uint8_t, constants::FifoSize>& fifoData)
{
    switch (fifoData[0]) {
        case BCSyncLost::getErrorCode():
            return std::make_shared<BCSyncLost>(fifoData);

        case FifoOverload::getErrorCode():
            return std::make_shared<FifoOverload>(fifoData);

        case PmEarlyHeader::getErrorCode():
            return std::make_shared<PmEarlyHeader>(fifoData);

        default:
            Print::PrintError("Unsupported GBT ERROR!");
            return std::make_shared<Unknown>(fifoData);
    }
}


Unknown::Unknown(const std::array<uint8_t, constants::FifoSize>& fifoData)
{
    size_t pos = 0;
    for(size_t idx = 0; idx < constants::FifoSize; ){
        data[pos++] = build32(fifoData.data(), idx);
    }
}

[[nodiscard]] WinCCResponse Unknown::createWinCCResponse()
{
    std::ofstream file(GbtErrorType::ErrorFile.data(), std::ios::app);

    std::time_t currentTime = std::time(nullptr);
    const std::tm* localTime = std::localtime(&currentTime);
    std::string timeStamp = std::string("[") + std::asctime(localTime) + std::string("]");

    file << timeStamp << "Unknown error report: ## IPbusWord" << std::endl;
    for(int idx = 0; idx < data.size(); idx++){
        file << timeStamp <<  std::setw(2) << idx << " " << std::hex << data[idx] << std::endl;
    }
    
    WinCCResponse respone;
    respone.addParameter("GBT_ERR_UNKNOWN", { 1 });
    return respone;
}

BCSyncLost::BCSyncLost(const std::array<uint8_t, constants::FifoSize>& fifoData)
{
    size_t index = 0;

    data.errorCode = build32(fifoData.data(), index);

    for (int i = 0; i < 10; ++i) {
        for(int p = 0; p < GbtWord::WordSize; p++){
            data.words[i].data.buffer[p] = build32(fifoData.data(),index);
        }

        data.words[i].counter = build16(fifoData.data(), index);
        data.words[i].isData  = fifoData[index++];
    }

    data.BCCRU       = build16(fifoData.data(),index);
    data.BCBoard     = build16(fifoData.data(),index);
    data.orbitBoard  = build32(fifoData.data(),index);
    data.orbitCRU    = build32(fifoData.data(),index);
    data.reservedSpace[0] = build32(fifoData.data(),index);
    data.reservedSpace[1] = build32(fifoData.data(),index);
}

WinCCResponse BCSyncLost::createWinCCResponse()
{
    std::ofstream file(GbtErrorType::ErrorFile.data(), std::ios::app);

    std::time_t currentTime = std::time(nullptr);
    const std::tm* localTime = std::localtime(&currentTime);
    std::string timeStamp = std::asctime(localTime);

    file << timeStamp << "Board BCid: " << std::hex << data.orbitBoard << " " << std::hex << data.BCBoard << std::endl;
    file << timeStamp << "\tCRU BCid: " << std::hex << data.orbitCRU << " " << std::hex << data.BCCRU << std::endl;
    file << timeStamp << "#### isData GBTword" << std::endl;
    for(int i = 0; i < 10; i++){
        file << timeStamp << std::setw(4) << data.words[i].counter << " " << std::setw(6) << (data.words[i].isData ? "true" : "false");
        for(int j = 0; j < GbtWord::WordSize; j++){
            file << std::hex << data.words[i].data.buffer[j] << ' ';
        }
        file << std::endl;
    }
     
    WinCCResponse response;
    response.addParameter(gbt::parameters::BCSyncLost.data(), { 1 });
    return std::move(response);
}

PmEarlyHeader::PmEarlyHeader(const std::array<uint8_t, constants::FifoSize>& fifoData)
{
    size_t index = 0;
    data.errorCode = build32(fifoData.data(), index);
    for(int i = 0; i < 14; i++){
        for(int p = 0; p < GbtWord::WordSize; p++){
            data.words[i].buffer[p] = build32(fifoData.data(), index);
        }
    }
}

WinCCResponse PmEarlyHeader::createWinCCResponse()
{
    std::ofstream file(GbtErrorType::ErrorFile.data(), std::ios::app);
    
    std::time_t currentTime = std::time(nullptr);
    const std::tm* localTime = std::localtime(&currentTime);
    std::string timeStamp = std::asctime(localTime);

    file << timeStamp <<"Input packet corrupted: header too early ## GBTword" << std::endl;
    for(int i = 0; i < 14; i++){
        file << timeStamp << std::setw(2) << i << " ";
        for(int j = 0; j < GbtWord::WordSize; j++){
            file << std::hex << data.words[i].buffer[j] << ' ';
        }
        file << std::endl;
    }

    WinCCResponse response;
    response.addParameter(gbt::parameters::PmEarlyHeader.data(), { 1 });
    return std::move(response);
}

FifoOverload::FifoOverload(const std::array<uint8_t, constants::FifoSize>& fifoData)
{
    size_t index = 0;

    data.errorCode = build32(fifoData.data(), index);
    for(int i = 0; i<13; i++){
        for(int p = 0; p < GbtWord::WordSize; p++){
            data.words[i].buffer[p] = build32(fifoData.data(), index);
        }
    }

    data.reservedSpace0 = build16(fifoData.data(), index);
    data.rdRate = build16(fifoData.data(), index);
    data.wrRate = build16(fifoData.data(), index);
    data._reservedSpace1 = build16(fifoData.data(), index);
}

WinCCResponse FifoOverload::createWinCCResponse()
{
    std::ofstream file(GbtErrorType::ErrorFile.data(), std::ios::app);
    
    std::time_t currentTime = std::time(nullptr);
    const std::tm* localTime = std::localtime(&currentTime);
    std::string timeStamp = std::asctime(localTime);

    file << timeStamp << "Raw data FIFO overload" << std::endl;
    file << timeStamp << data.rdRate << " read and " << data.wrRate << " operations in last 1000 cycles" << std::endl;
    file << timeStamp << "## GBT word" << std::endl;
    for(int idx = 0; idx < 13; idx++){
        file << timeStamp << std::setw(2) << idx << " ";
        for(int j = 0; j < GbtWord::WordSize; j++){
            file << std::hex << data.words[idx].buffer[j] << ' ';
        }
    }

    WinCCResponse response;
    response.addParameter(gbt::parameters::FifoOverload.data(), { 1 });
    return std::move(response);
}

} // namespace gbt

namespace gbt
{
WinCCResponse GbtRateMonitor::updateRates(uint32_t wordsCount, uint32_t eventsCount)
{
    WinCCResponse response;
    response.addParameter(parameters::WordsRate.data(), { m_wordsRate.updateRate(wordsCount, m_timeInterval) });
    response.addParameter(parameters::EventsRate.data(), { m_eventsRate.updateRate(eventsCount, m_timeInterval) });
    return response;
}

double GbtRate::updateRate(uint32_t newCount, std::chrono::milliseconds timeInterval)
{
    double rate = (newCount >= oldCount) ? static_cast<double>(newCount - oldCount) / static_cast<double>(timeInterval.count()) : newCount;
    oldCount = newCount;
    return rate;
}

} // namespace gbt