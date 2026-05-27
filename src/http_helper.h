#pragma once
#include <Arduino.h>

bool SendJsonPost(const String& targetName, const String& url, const String& json);
