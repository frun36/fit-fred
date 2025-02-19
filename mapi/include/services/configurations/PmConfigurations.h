#pragma once

#include "services/configurations/BoardConfigurations.h"
#include "Fred/Mapi/mapi.h"

class PmConfigurations : public Mapi, public BoardConfigurations
{
   public:
    PmConfigurations(std::shared_ptr<Board> board);
    string processInputMessage(string msg) override;
    string processOutputMessage(string msg) override;

    const string& getServiceName() const override { return name; }
};
