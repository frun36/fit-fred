#include "mapifactory.h"

#include "basic_mapi/mapiexample.h"
#include "indefinite_mapi/indefinitemapi.h"
#include "iterative_mapi/averagetemp.h"
#include "mapigroup/testgroup.h"

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
    
}

