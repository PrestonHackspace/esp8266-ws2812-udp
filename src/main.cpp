#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiManager.h>

#include "config.h"
#include "mdns-helper.h"
#include "mqtt-helper.h"
#include "espshow.h"

WiFiUDP Udp;
char incomingPacket[255]; // buffer for incoming packets

ESP8266WebServer httpServer(HTTP_PORT);
ESP8266HTTPUpdateServer httpUpdater;

String deviceReplyChannel;

uint8_t InitPattern[] = {255, 0, 0, 0, 255, 0, 0, 0, 255};
uint8_t ReadyPattern[] = {255, 255, 255, 0, 0, 0, 0, 0, 0};
uint8_t WarnPattern[] = {0, 255, 0, 0, 255, 0, 0, 255, 0};

void connect_udp(IPAddress ip, uint16_t localUdpPort)
{
  Serial.printf("Connecting to multicast IP %s, UDP port %d\n", ip.toString().c_str(), localUdpPort);
  Udp.beginMulticast(WiFi.localIP(), ip, localUdpPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);
}

String commandHandler(String msg)
{
  if (msg == "reset")
  {
    ESP.reset();
  }

  if (msg == "ping")
  {
    return String("pong");
  }

  if (msg.startsWith("connect-udp"))
  {
    IPAddress ip;

    if (ip.fromString(msg.substring(12)))
    {
      connect_udp(ip, 54321);
    }
  }

  return String("");
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  pinMode(LED_PIN, OUTPUT);

  noInterrupts();
  espShow(LED_PIN, InitPattern, 9, true);
  interrupts();

  auto deviceName = getDeviceName();

  WiFiManager wifiManager;

  wifiManager.autoConnect((deviceName + " (" + DEVICE_TYPE_NAME + ")").c_str());

  MDNS.begin(deviceName.c_str());

  httpUpdater.setup(&httpServer);

  httpServer.on(String("/"), [deviceName]() {
    String json("{\"deviceType\":\"DEVICE_TYPE\",\"deviceName\":\"DEVICE_NAME\",\"version\":\"VERSION\"}");

    json.replace("DEVICE_TYPE", DEVICE_TYPE);
    json.replace("DEVICE_NAME", deviceName);
    json.replace("VERSION", VERSION);

    httpServer.send(200, "application/json", json);
  });

  httpServer.on(String("/cmd"), []() {
    String msg = httpServer.arg(0);

    String reply = commandHandler(msg);

    httpServer.send(200, "text/plain", reply);
  });

  httpServer.begin();

  MDNS.addService("http", "tcp", HTTP_PORT);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local:%d/update in your browser\n", deviceName.c_str(), HTTP_PORT);

  String deviceChannel = String(DEVICE_CHANNEL_PREFIX) + "in/" + deviceName;
  String typeChannel = String(DEVICE_CHANNEL_PREFIX) + "in/" + DEVICE_TYPE;
  String allChannel = String(DEVICE_CHANNEL_PREFIX) + "in/all";

  deviceReplyChannel = String(DEVICE_CHANNEL_PREFIX) + "out/" + deviceName;

  MdnsService mqttService;
  if (mdns_discover("mqtt", 5, &mqttService))
  {
    mqtt_init(deviceName.c_str(), mqttService.ip, mqttService.port);

    mqtt_subscribe(deviceChannel.c_str());
    mqtt_subscribe(typeChannel.c_str());
    mqtt_subscribe(allChannel.c_str());

    mqtt_callback([](const char *topic, uint8_t *payload, unsigned int length) {
      char *message = new char[length + 1];

      memcpy(message, payload, length);
      message[length] = 0;

      String msg = String(message);

      String reply = commandHandler(msg);

      if (reply.length() > 0)
      {
        mqtt_publish(deviceReplyChannel.c_str(), reply.c_str());
      }

      delete message;
    });

    mqtt_publish((String(REGISTER_CHANNEL_PREFIX) + DEVICE_TYPE).c_str(), deviceName.c_str());
  }

  if (mqtt_isconnected())
  {
    noInterrupts();
    espShow(LED_PIN, ReadyPattern, 9, true);
    interrupts();
  }
  else
  {
    Serial.println("WARNING: MQTT not connected. Will reboot in 30 seconds...");

    noInterrupts();
    espShow(LED_PIN, WarnPattern, 9, true);
    interrupts();
  }
}

void loop()
{
  // Check MQTT connected state...
  // Could choose to the keep the program running, but for this application MQTT is required.
  // So reboot after 30 seconds and hope that MQTT is found the next time.

  if (!mqtt_isconnected())
  {
    if (millis() > 30 * 1000)
    {
      ESP.reset();
    }
  }

  httpServer.handleClient();

  mqtt_loop();

  int packetSize = Udp.parsePacket();

  if (packetSize)
  {
    int len = Udp.read(incomingPacket, 255);

    noInterrupts();
    espShow(LED_PIN, (uint8_t *)incomingPacket, len, true);
    interrupts();
  }
}