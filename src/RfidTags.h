#pragma once
#include <Arduino.h>

struct RfidTag
{
    String rawData;
};

struct RfidTagPosition
{
    int x;
    int y;
    int lv;
};

struct RfidTagObject
{
    RfidTag tag;
    RfidTagPosition position;
};