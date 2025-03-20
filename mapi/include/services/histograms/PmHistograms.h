#pragma once

#include <sstream>
#include "services/histograms/BoardHistograms.h"
#include "board/FitData.h"
#include "services/histograms/PmHistogramController.h"

class PmHistograms : public BoardHistograms
{
   private:
    static constexpr useconds_t ReadoutInterval = 1'000'000;
    uint32_t m_fifoAddress;
    PmHistogramController m_controller;

   public:
    PmHistograms(shared_ptr<Board> pm, std::unordered_map<std::string, FitData::PmHistogram> histograms);

    Result<std::string, std::string> readAndStoreHistograms() override;
    Result<std::string, std::string> resetHistograms() override;
    void parseResponse(ostringstream& oss) const override;
};
