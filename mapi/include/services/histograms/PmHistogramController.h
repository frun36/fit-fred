#pragma once

#include "board/BoardCommunicationHandler.h"
#include "services/histograms/PmHistogramData.h"
#include "services/templates/BasicFitIndefiniteMapi.h"
#include "utils/utils.h"
#include <vector>
#include <string>

class PmHistogramController
{
   private:
    BasicFitIndefiniteMapi& m_mapi;
    PmHistogramData m_data;
    BoardCommunicationHandler m_handler;
    const size_t m_fifoAddress;

   public:
    PmHistogramController(BasicFitIndefiniteMapi& mapi, shared_ptr<Board> pm, std::unordered_map<std::string, FitData::PmHistogram> histograms);

    Result<std::string, std::string> selectHistograms(const std::vector<std::string>& names);
    Result<std::string, std::string> resetHistograms();
    Result<std::string, std::string> switchHistogramming(bool on);
    Result<std::string, std::string> setBcIdFilter(int64_t bcId);

    Result<std::string, std::string> readAndStoreHistograms();

    inline const BlockView& getData() const {
        return m_data.getData();
    }
};
