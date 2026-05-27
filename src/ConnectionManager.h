#pragma once
#include <Arduino.h>
#include <map>
#include <optional>

const String deviceName = "rescuecar-esp32";
const String carName = "rescuecar";
const String orangepiIp = "http://192.168.10.10";

class ConnectionManager
{
private:
    std::map<String, String> connectedDevices;
public:
    void AddConnection(String name, String ip);
    String GetIp(String name);

    void LogInToOrangepi();
    void TryGetCarIp();
};