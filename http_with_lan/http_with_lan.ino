#include <SPI.h>
#include <Ethernet.h>

static const uint8_t D0 = 16;
static const uint8_t D1 = 5;
static const uint8_t D2 = 4;
static const uint8_t D3 = 0;
static const uint8_t D4 = 2;
static const uint8_t D5 = 14;
static const uint8_t D6 = 12;
static const uint8_t D7 = 13;
static const uint8_t D8 = 15;
static const uint8_t D9 = 3;
static const uint8_t D10 = 1;

#define LED_PIN D2

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 109);
IPAddress dns(8, 8, 8, 8);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

IPAddress serverIP(185, 78, 166, 46);
int serverPort = 3133;

EthernetClient client;

void setup() {
  pinMode(LED_PIN, OUTPUT);

  Ethernet.init(5);
  Ethernet.begin(mac, ip, dns, gateway, subnet);

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println();
  Serial.println(Ethernet.localIP());
}

void loop() {
  Serial.println("Start http post");

  sendToAPI("0123456789");

  Serial.println("End http post");
}

void sendToAPI(String sn) {
  String url = "/api/canteen/buffet";
  String body = "{\"sn\": \"" + sn + "\", \"machine\": \"3\"}";

  Serial.println("Sending payload:");
  Serial.println(body);

  if (client.connect(serverIP, serverPort)) {
    Serial.println("Connected to server");

    client.println("POST " + url + " HTTP/1.1");
    client.println("Host: " + serverIP.toString());
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(body.length());
    client.println();
    client.println(body);

    bool success = false;
    while (client.connected()) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        Serial.println(line);

        if (line.startsWith("HTTP/1.1 ")) {
          int statusCode = line.substring(9, 12).toInt();
          Serial.print("Response status code: ");
          Serial.println(statusCode);

          if (statusCode == 200 || statusCode == 201) {
            Serial.println("Success");
            success = true;
            digitalWrite(LED_PIN, HIGH);
            delay(2000);
            digitalWrite(LED_PIN, LOW);
          } else {
            Serial.println("No money");
          }
          break;
        }
      }
    }

    if (!success) {
      Serial.println("Failed");
    }

    client.stop();
    Serial.println("Disconnected from server");
  } else {
    Serial.println("Connection failed");
  }
}
