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

ESP8266WebServer httpServer(8080);
ESP8266HTTPUpdateServer httpUpdater;

void connect_udp(IPAddress ip, uint16_t localUdpPort)
{
  Serial.printf("Connecting to multicast IP %s, UDP port %d\n", ip.toString().c_str(), localUdpPort);
  Udp.beginMulticast(WiFi.localIP(), ip, localUdpPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  auto deviceName = getDeviceName();

  WiFiManager wifiManager;

  wifiManager.autoConnect((deviceName + " (" + DEVICE_TYPE_NAME + ")").c_str());

  pinMode(LED_PIN, OUTPUT);

  MDNS.begin(deviceName.c_str());

  httpUpdater.setup(&httpServer);

  httpServer.on(String('/'), [deviceName]() {
    String json("{\"deviceType\":\"DEVICE_TYPE\",\"deviceName\":\"DEVICE_NAME\"}");

    json.replace("DEVICE_TYPE", DEVICE_TYPE);
    json.replace("DEVICE_NAME", deviceName);

    httpServer.send(200, "application/json", json);
  });

  httpServer.begin();

  MDNS.addService("http", "tcp", 8080);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local:8080/update in your browser\n", deviceName.c_str());

  String deviceChannel = String(DEVICE_CHANNEL_PREFIX) + "in/" + deviceName;
  String typeChannel = String(DEVICE_CHANNEL_PREFIX) + "in/" + DEVICE_TYPE;
  String allChannel = String(DEVICE_CHANNEL_PREFIX) + "in/all";

  String deviceReplyChannel = String(DEVICE_CHANNEL_PREFIX) + "out/" + deviceName;

  MdnsService mqttService;
  if (mdns_discover("mqtt", 5, &mqttService))
  {
    mqtt_init(deviceName.c_str(), mqttService.ip, mqttService.port);

    mqtt_subscribe(deviceChannel.c_str());
    mqtt_subscribe(typeChannel.c_str());
    mqtt_subscribe(allChannel.c_str());

    mqtt_callback([deviceReplyChannel](const char *topic, uint8_t *payload, unsigned int length) {
      char *message = new char[length + 1];

      memcpy(message, payload, length);
      message[length] = 0;

      String msg = String(message);

      if (msg == "reset")
      {
        ESP.reset();
      }

      if (msg == "ping")
      {
        mqtt_publish(deviceReplyChannel.c_str(), "pong");
      }

      if (msg.startsWith("connect-udp"))
      {
        IPAddress ip;

        if (ip.fromString(msg.substring(12)))
        {
          connect_udp(ip, 54321);
        }
      }

      delete message;
    });

    mqtt_publish((String(REGISTER_CHANNEL_PREFIX) + DEVICE_TYPE).c_str(), deviceName.c_str());
  }
}

void loop()
{
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