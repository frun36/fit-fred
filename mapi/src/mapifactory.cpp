#include "mapifactory.h"
#include "FitData.h"

MapiFactory::MapiFactory(Fred *fred) {
    this->fred = fred;

    try {
        this->generateObjects();
    } catch (const exception &e) {
        Print::PrintError(e.what());
        exit(EXIT_FAILURE);
    }
}

MapiFactory::~MapiFactory() {
    for (size_t i = 0; i < this->mapiObjects.size(); i++) {
        delete this->mapiObjects[i];
    }
}

void MapiFactory::generateObjects() {
    FitData boardsData;
    if(boardsData.isReady() == false)
    {
        Print::PrintError("Configuration failed! Aborting");
    }
    Print::PrintVerbose("Registering MAPI Objects");

    
    for(auto board: boardsData.getBoards())
    {
        if(board.first.find("TCM") != std::string::npos)
        {
            parametersObject.emplace_back(board.second);
            this->fred->registerMapiObject(this->fred->Name() + "/TCM/" + board.first + "/PARAMETERS", (Mapi*) & parametersObject.back());
        }
        else
        {
            parametersObject.emplace_back(board.second);
            this->fred->registerMapiObject(this->fred->Name() + "/PM/" + board.first + "/PARAMETERS", (Mapi*) & parametersObject.back());
        }
        Print::PrintVerbose(board.first + " registered");
    }
}

