#include "services/histograms/PmHistograms.h"
#include <cstdio>
#include <sstream>
#include <string>
#include "services/histograms/BinBlock.h"
#include "utils/utils.h"

PmHistograms::PmHistograms(shared_ptr<Board> pm, std::unordered_map<std::string, FitData::PmHistogram> histograms)
    : m_controller(pm, histograms)
{
    addOrReplaceHandler("SELECT", [this](vector<string> arguments) -> Result<string, string> {
        return m_controller.selectHistograms(arguments);
    });

    addOrReplaceHandler("HISTOGRAMMING", [this](vector<string> arguments) -> Result<string, string> {
        if (arguments.size() != 1 || (arguments[0] != "0" && arguments[0] != "1")) {
            return { .ok = nullopt, .error = "HISTOGRAMMING command takes exactly one argument: 0 or 1" };
        }
        return m_controller.switchHistogramming(*this, arguments[0] == "1");
    });

    addOrReplaceHandler("BCID_FILTER", [this](vector<string> arguments) -> Result<string, string> {
        int counterId;
        istringstream iss(arguments.size() == 1 ? arguments[0] : "");

        if (arguments[0] == "OFF") {
            return m_controller.setBcIdFilter(*this, -1);
        }

        if (!(iss >> counterId) || !iss.eof()) {
            return { .ok = nullopt, .error = "BCID_FILTER takes exactly one valid integer argument or the string \"OFF\"" };
        }

        return m_controller.setBcIdFilter(*this, counterId);
    });
}

Result<std::string, std::string> PmHistograms::readAndStoreHistograms()
{
    return m_controller.readAndStoreHistograms(*this);
}

void PmHistograms::parseResponse(ostringstream& oss) const
{
    for (const auto& [name, blocks] : m_controller.getData()) {
        oss << name.c_str();
        for (const BinBlock* block : blocks) {
            if (!block->readoutEnabled) {
                continue;
            }
            if (block->binsPerRegister != 2) {
                oss << block->binsPerRegister;
                break;
            }
            if (block->isNegativeDirection) {
                for (auto it = block->data.rbegin(); it != block->data.rend(); ++it) {
                    oss << "," << ((*it) >> 16) << "," << ((*it) & 0xFFFF);
                }
            } else {
                for (auto it = block->data.begin(); it != block->data.end(); ++it) {
                    oss << "," << ((*it) & 0xFFFF) << "," << ((*it) >> 16);
                }
            }
        }
        oss << "\n";
    }
}
