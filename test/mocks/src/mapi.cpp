#include "../include/mapi.h"

Mapi::Mapi() : returnError(false), noReturn(false), noRpcRequest(false) {}
void Mapi::registerMapi([[maybe_unused]] Fred* fred, [[maybe_unused]] string name) {}

void Iterativemapi::newRequest([[maybe_unused]] string request) {}
void Iterativemapi::publishAnswer([[maybe_unused]] string message) {}
void Iterativemapi::publishError([[maybe_unused]] string error) {}

void Mapigroup::publishAnswer([[maybe_unused]] string message) {}
void Mapigroup::publishError([[maybe_unused]] string error) {}
void Mapigroup::newMapiGroupRequest([[maybe_unused]] vector<pair<string, string>> requests) {}
void Mapigroup::newTopicGroupRequest([[maybe_unused]] vector<pair<string, string>> requests) {}

void IndefiniteMapi::publishAnswer([[maybe_unused]] string message) {}
void IndefiniteMapi::publishError([[maybe_unused]] string error) {}
string IndefiniteMapi::processInputMessage([[maybe_unused]] string msg) { return ""; }
string IndefiniteMapi::processOutputMessage([[maybe_unused]] string msg) { return ""; }
string IndefiniteMapi::executeAlfSequence([[maybe_unused]] string seq) { return ""; }
string IndefiniteMapi::waitForRequest([[maybe_unused]] bool& running) { return ""; }
