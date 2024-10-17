#pragma once

#include <vector>
#include <string>

using namespace std;

class Utility
{
   public:
    // static vector<double> splitString2Num(const string& text, string by);
    static vector<string> splitString(const string& text, string by);
    // static vector<vector<string> > splitMessage(const string& text);
    // static vector<vector<double> > splitMessage2Num(const string& text);
    static void removeWhiteSpaces(string& text);
    // static void removeComment(string& text);
    // static void printVector(const vector<string>& data);
    // static void printVector(const vector<int32_t>& data);
    static double calculateEquation(string& equation, vector<string>& variables, vector<double>& values);
    // static string readbackToString(vector<double> data);
    // static void checkMessageIntegrity(const string& request, const string& response, Instructions::Type type);
    // static uint128_t stringToLargeNumber(const string& number);
    // static uint32_t stringToNumber(const string& number);
    // static string largeNumberToHexString(const uint128_t number, bool addZeros = false);

   private:
    Utility();
};