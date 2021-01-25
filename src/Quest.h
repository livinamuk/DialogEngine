#pragma once
#include <string>
#include <vector>
#include "Header.h"

class Quest {
public:
    // Fields
    std::string m_name;
    QuestState m_questState;

    // Methods
    GameFloat(std::string name, float value);
    void SetTo(float value);
    void Add(float value);
    void Subract(float value);
    void SetMin(float value);
    void SetMax(float value);
    void LimitWithinRange();
};