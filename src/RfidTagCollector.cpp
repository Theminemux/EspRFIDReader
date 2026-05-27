#include "RfidTagCollector.h"

RfidTagCollector::RfidTagCollector()
{
}

void RfidTagCollector::addTag(const RfidTag& tag)
{
    tags.push_back(tag); // Add new tag to the collection
}