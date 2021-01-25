#pragma once
#include <string>
#include <sstream> 
#include <vector>

class Util 
{
public:

    // Static Methods

    static int StringToInt(std::string str);
    static float StringToFloat(std::string str);
    static std::string RemoveFromBeginning(std::string str, int numberOfCharacters);
};

