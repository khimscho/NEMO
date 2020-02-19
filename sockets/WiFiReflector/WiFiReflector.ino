#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

const char *ssid = "sb2030";
const char *password = "sb2030logger";

const int port = 25443;

WiFiServer reflector(port);
int message = 0;
String buffer = "";
int loop_counter = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Configuring access point ...");

  WiFi.softAP(ssid, password);
  IPAddress server_address = WiFi.softAPIP();
  Serial.print("AP address: ");
  Serial.println(server_address);
  reflector.begin();

  Serial.println("Server started");
}

void loop() {
  static WiFiClient client = reflector.available();

  loop_counter ++;
  if (client) {
    if (client.connected()) {
      while (client.available() > 0) {
        char c = client.read();
        if (c == '\n') {
          Serial.println("Received: " + buffer);
          buffer = "Reply " + String(message);
          message ++;
          client.println(buffer);
          Serial.println("Sent: " + buffer);
          buffer = "";
        } else {
          buffer += c;
        }
      }
    } else {
      client.stop();
      Serial.println("Client disconnected.");
    }
  } else {
    client = reflector.available();
    if (client) {
      Serial.println("Client connected.");
    }
  }
}
