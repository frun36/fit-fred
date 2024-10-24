#pragma once

#include "Fred/fred.h"
#include "Fred/Mapi/mapi.h"
#include "Alfred/print.h"
#include "services/Parameters.h"
#include "services/BoardStatus.h"
#include "services/Configurations.h"
#include "services/ResetFEE.h"
#include "services/Reset.h"
#include "services/ResetErrors.h"
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>

class MapiFactory
{
   private:
    Fred* m_fred;

    vector<Mapi*> m_mapiObjects;

    std::list<Parameters> m_parametersObjects;
    std::list<BoardStatus> m_statusObjects;
    std::list<Reset> m_resetObjects;
    Configurations m_configurationsObject;

    std::shared_ptr<ResetFEE> m_resetSystem;
    std::shared_ptr<ResetErrors> m_resetError;
    void generateObjects();

   public:
    MapiFactory(Fred* fred);
    virtual ~MapiFactory();
};
