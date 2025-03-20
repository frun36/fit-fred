#include <string>
#include <unordered_map>
#include "services/templates/BasicFitIndefiniteMapi.h"
#include "services/calibration/utils.h"

namespace calibration
{

struct Calibrator {
    using Params = std::unordered_map<std::string, double>;

    virtual Result<string, string> run(BasicFitIndefiniteMapi& mapi, ChannelArray<bool> calibChannelMask, const Params params) = 0;
};

}; // namespace calibration
