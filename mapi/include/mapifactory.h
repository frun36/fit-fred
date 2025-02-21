#pragma once

#include "Fred/fred.h"
#include "Fred/Mapi/mapi.h"
#include "services/Parameters.h"
#include "services/BoardStatus.h"
#include "services/Configurations.h"
#include "services/ResetFEE.h"
#include "services/Reset.h"
#include "services/ResetErrors.h"
#include "services/SetPhaseDelay.h"
#include "services/CounterRates.h"
#include "services/ConfigurationDatabaseBroker.h"
#include "services/histograms/PmHistograms.h"
#include "services/histograms/TcmHistograms.h"
#include <memory>

class MapiFactory
{
   private:
    Fred* m_fred;

    std::list<Parameters> m_parametersObjects;
    std::list<BoardStatus> m_statusObjects;
    std::list<Reset> m_resetObjects;
    std::list<CounterRates> m_counterRatesObjects;
    std::list<PmHistograms> m_pmHistogramsObjects;

    std::unique_ptr<Configurations> m_configurationsObject;
    std::unique_ptr<ResetFEE> m_resetSystem;
    std::unique_ptr<ResetErrors> m_resetError;
    std::unique_ptr<SetPhaseDelay> m_setPhaseDelay;
    std::unique_ptr<ConfigurationDatabaseBroker> m_saveConfiguration;

    std::unique_ptr<TcmHistograms> m_tcmHistograms;

    void generateObjects();

   public:
    MapiFactory(Fred* fred);
    virtual ~MapiFactory() {}
};
