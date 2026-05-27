#pragma once

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>

class HttpRequests
{
public:
    bool SendJsonPost(const String& targetName, const String& url, const String& json);
};