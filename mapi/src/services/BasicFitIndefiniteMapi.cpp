#include "services/BasicFitIndefiniteMapi.h"

const BoardCommunicationHandler::ParsedResponse BasicFitIndefiniteMapi::EmptyResponse({ WinCCResponse(), {} });
const BoardCommunicationHandler::FifoResponse BasicFitIndefiniteMapi::EmptyFifoResponse{ {}, nullopt };