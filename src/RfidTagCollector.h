#pragma once
#include <Arduino.h>
#include <vector>
#include "RfidTags.h"

class RfidTagCollector
{
private:
    std::vector<RfidTag> tags; // Collection of RFID tags
public:
    RfidTagCollector();
    void addTag(const RfidTag& tag);
};