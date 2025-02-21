#pragma once

#include "services/configurations/BoardConfigurations.h"
#include "services/templates/BasicFitIndefiniteMapi.h"

class TcmConfigurations : public BasicFitIndefiniteMapi, public BoardConfigurations
{
   public:
    TcmConfigurations(std::shared_ptr<Board> board);

    void processExecution() override;

   private:
    string m_response;

    bool handleDelays();
    bool handleData();
    void handleResetErrors();

    const string& getServiceName() const override { return name; }
};
