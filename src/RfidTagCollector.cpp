#include "RfidTagCollector.h"
#include <ArduinoJson.h>

RfidObjectCollector::RfidObjectCollector()
{
}

void RfidObjectCollector::addTag(const RfidTagObject& tag)
{
    tags.push_back(tag); // Add new tag to the collection
}

String RfidObjectCollector::getAllTagsAsJson() const
{
    JsonDocument doc;
    JsonArray tagsArray = doc.to<JsonArray>();
    
    for (const auto& tag : tags) {
        JsonObject tagObj = tagsArray.createNestedObject();
        tagObj["rawData"] = tag.tag.rawData;
        tagObj["x"] = tag.position.x;
        tagObj["y"] = tag.position.y;
        tagObj["lv"] = tag.position.lv;
    }

    String json;
    serializeJson(doc, json);
    return json;
}