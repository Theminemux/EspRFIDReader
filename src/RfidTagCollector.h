#pragma once
#include <Arduino.h>
#include <vector>
#include "RfidTags.h"

class RfidObjectCollector
{
private:
    std::vector<RfidTagObject> tags; // Collection of RFID tags
public:
    RfidObjectCollector();
    void addTag(const RfidTagObject& tag);
    String getAllTagsAsJson() const;
};