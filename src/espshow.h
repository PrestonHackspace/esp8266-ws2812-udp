#pragma once

#include <Arduino.h>

#if defined(ESP8266)
// ESP8266 show() is external to enforce ICACHE_RAM_ATTR execution
extern "C" void ICACHE_RAM_ATTR espShow(
    uint8_t pin, uint8_t *pixels, uint32_t numBytes, uint8_t type);
#elif defined(ESP32)
extern "C" void espShow(
    uint8_t pin, uint8_t *pixels, uint32_t numBytes, uint8_t type);
#endif // ESP8266
