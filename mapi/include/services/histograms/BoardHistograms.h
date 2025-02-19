#pragma once

#include <chrono>
#include <sstream>
#include "services/LoopingFitIndefiniteMapi.h"
#include "utils.h"

class BoardHistograms : public LoopingFitIndefiniteMapi
{
   private:
    ostringstream m_responseOss;
    uint32_t m_readId = 0;

    static constexpr useconds_t ReadoutInterval = 1'000'000;

    Result<chrono::duration<double, milli>, string> handleReadout();
    void handleResponse(double readElapsed, string requestResultString = "");
    void processExecution() override;

   protected:
    BoardHistograms();

    virtual Result<string, string> resetHistograms() = 0;
    virtual Result<string, string> readAndStoreHistograms() = 0;
    virtual void parseResponse(ostringstream& oss) const = 0;

    virtual ~BoardHistograms() = default;
};
