#include "mdns-helper.h"

bool mdns_discover(const char serviceName[], uint8_t attempts, MdnsService *service)
{
  auto retries = attempts;

  while (retries > 0)
  {
    Serial.println("Sending mDNS query");
    int n = MDNS.queryService(serviceName, "tcp"); // Send out query for esp tcp services
    Serial.println("mDNS query done");

    if (n == 0)
    {
      Serial.println(String("no ") + serviceName + String(" services found, retrying..."));
      delay(1000);
      retries--;
    }
    else
    {
      Serial.println(String(n) + String(" service(s) found"));

      for (int i = 0; i < n; ++i)
      {
        auto hostname = MDNS.hostname(i);
        auto ip = MDNS.IP(i);
        auto port = MDNS.port(i);

        // Print details for each service found
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(hostname);
        Serial.print(" (");
        Serial.print(ip);
        Serial.print(":");
        Serial.print(port);
        Serial.println(")");

        service->ip = ip;
        service->port = port;

        return true;
      }
    }
  }

  Serial.println("MDNS giving up");

  return false;
}
