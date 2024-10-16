#pragma once

#include "Fred/fred.h"
#include "Fred/Mapi/mapi.h"
#include "Alfred/print.h"
#include "services/Parameters.h"
#include "services/BoardStatus.h"
#include "services/Configurations.h"
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
    std::vector<Parameters> m_parametersObjects;
    std::list<BoardStatus> m_statusObjects;
    Configurations m_configurationsObject;

    void generateObjects();

   public:
    MapiFactory(Fred* fred);
    virtual ~MapiFactory();
};
