#pragma once

#include <Arduino.h>

extern const char *DEVICE_TYPE;
extern const char *DEVICE_TYPE_NAME;
extern const char *REGISTER_CHANNEL_PREFIX;
extern const char *DEVICE_CHANNEL_PREFIX;

extern const int LED_PIN;

String getDeviceName();
