#pragma once

#include <ESP8266mDNS.h>

struct MdnsService
{
  IPAddress ip;
  uint16_t port;
};

bool mdns_discover(const char serviceName[], uint8_t attempts, MdnsService *service);
