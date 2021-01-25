#pragma once
#include "Util.h"

int Util::StringToInt(std::string str)
{
    if (str.length() == 0)
        return 0;

    std::stringstream stringStream(str);
    int result = 0;
    stringStream >> result;
    return result;
}

float Util::StringToFloat(std::string str)
{
    if (str.length() == 0)
        return 0;

    return std::stof(str);
}

std::string Util::RemoveFromBeginning(std::string str, int numberOfCharacters)
{
    return str.substr(numberOfCharacters, str.length() - 1);
}