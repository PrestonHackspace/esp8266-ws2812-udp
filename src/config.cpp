#include "config.h"

const char *DEVICE_TYPE = "led-strip";
const char *DEVICE_TYPE_NAME = "LED Strip";
const char *REGISTER_CHANNEL_PREFIX = "house/register/";  // ... [DEVICE_TYPE]
const char *DEVICE_CHANNEL_PREFIX = "house/device/";      // ... [in/out] / [Device]

const int LED_PIN = 2;

String getDeviceName()
{
  return "ESP-" + String(ESP.getChipId(), 16);
}
