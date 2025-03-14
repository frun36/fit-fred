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
    PmHistogramData m_data;
    BoardCommunicationHandler m_handler;
    const size_t m_fifoAddress;

   public:
    PmHistogramController(shared_ptr<Board> pm, std::unordered_map<std::string, FitData::PmHistogram> histograms);

    Result<std::string, std::string> selectHistograms(const std::vector<std::string>& names);
    Result<std::string, std::string> resetHistograms(BasicFitIndefiniteMapi& mapi);
    Result<std::string, std::string> switchHistogramming(BasicFitIndefiniteMapi& mapi, bool on);
    Result<std::string, std::string> setBcIdFilter(BasicFitIndefiniteMapi& mapi, int64_t bcId);

    Result<std::string, std::string> readAndStoreHistograms(BasicFitIndefiniteMapi& mapi);

    inline const BlockView& getData() const {
        return m_data.getData();
    }
};
