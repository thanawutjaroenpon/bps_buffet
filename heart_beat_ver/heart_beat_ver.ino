#include <SPI.h>
#include <MFRC522.h>
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
#define BUZZER D0

#define SS_PIN D4
#define RST_PIN D3


/*
22:33:CA:76:5E:06  device 1
0x22, 0x33, 0xCA, 0x76, 0x5E, 0x06 


83:7A:E6:EF:7A:42  device 2
0x83, 0x7A, 0xE6, 0xEF, 0x7A, 0x43


5B:C2:69:23:C0:90  device 3j
0x5B, 0xC2, 0x69, 0x23, 0xC0, 0x90

*/
byte mac[] = {0x5B, 0xC2, 0x69, 0x23, 0xC0, 0x90};
IPAddress ip(192, 168, 95, 1);
IPAddress dns(172, 17, 5, 227);
IPAddress gateway(192, 168, 95, 254);
IPAddress subnet(255, 255, 255, 0);
String gate = "SS Gate 1";

IPAddress serverIP(172, 17, 5, 83);
int serverPort = 3000;

EthernetClient client;
MFRC522 rfid(SS_PIN, RST_PIN);

unsigned long previousMillis = 0;
const long interval = 300000; // 60 seconds for heartbeat

bool heartbeatPending = false;
bool apiPending = false;
String apiData = "";

void setup() {
  Serial.begin(9600);
  while (!Serial) { ; }

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  Ethernet.init(5);
  Ethernet.begin(mac, ip, dns, gateway, subnet);

  Serial.println();
  Serial.println(Ethernet.localIP());

  SPI.begin();
  rfid.PCD_Init();
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    heartbeatPending = true;
  }

  if (heartbeatPending) {
    sendHeartbeat();
  }

  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  Serial.println(F("A new card has been detected."));
  String rfid_in = rfid_read();
  Serial.println(rfid_in);
  apiData = rfid_in;
  apiPending = true;

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  digitalWrite(BUZZER, LOW);

  if (apiPending) {
    sendToAPI(apiData);
  }
}

void sendHeartbeat() {
  String url = "/api/machine/detect";
  String ipAddress = Ethernet.localIP().toString();
  String macAddress = "";

  for (byte i = 0; i < 6; i++) {
    if (mac[i] < 0x10) {
      macAddress += "0";
    }
    macAddress += String(mac[i], HEX);
    if (i < 5) {
      macAddress += ":";
    }
  }

  String body = "{\"device\": \"" + gate + "\", \"ip\": \"" + ipAddress + "\", \"mac\": \"" + macAddress + "\"}";

  // Print the data being sent
  Serial.println("Sending Heartbeat Data:");
  Serial.println(body);

  if (client.connect(serverIP, serverPort)) {
    client.println("POST " + url + " HTTP/1.1");
    client.println("Host: " + serverIP.toString());
    client.println("Content-Type: application/json");
    client.print("Content-Length: ");
    client.println(body.length());
    client.println();
    client.println(body);

    while (client.connected()) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        if (line.startsWith("HTTP/1.1 ")) {
          int statusCode = line.substring(9, 12).toInt();
          if (statusCode == 200) {
            Serial.println("Heartbeat sent successfully");
            heartbeatPending = false;
          }
          break;
        }
      }
    }
    client.stop();
  } else {
    Serial.println("Heartbeat failed, device might be offline");
    heartbeatPending = false;
  }
}


void sendToAPI(String sn) {
  String url = "/api/canteen/buffet";
  String body = "{\"sn\": \"" + sn + "\", \"machine\": \"" + gate + "\"}";

  if (client.connect(serverIP, serverPort)) {
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
            digitalWrite(BUZZER, HIGH);
          } else {
            Serial.println("No money");
            digitalWrite(BUZZER, HIGH);
            delay(1000);
            digitalWrite(BUZZER, LOW);
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
    apiPending = false;
  } else {
    Serial.println("Connection failed");
    digitalWrite(BUZZER, HIGH);
    delay(300);
    digitalWrite(BUZZER, LOW);
    delay(300);
    digitalWrite(BUZZER, HIGH);
    delay(300);
    digitalWrite(BUZZER, LOW);
    delay(300);
    digitalWrite(BUZZER, HIGH);
    delay(300);
    digitalWrite(BUZZER, LOW);
    apiPending = false;
  }
}

String rfid_read() {
  String content = "";
  for (int i = rfid.uid.size - 1; i >= 0; i--) {
    content.concat(String(rfid.uid.uidByte[i] < 0x10 ? "0" : ""));
    content.concat(String(rfid.uid.uidByte[i], HEX));
  }
  content.toUpperCase();

  unsigned long decimalValue = strtoul(content.c_str(), NULL, 16);
  String decimalString = String(decimalValue);

  while (decimalString.length() < 10) {
    decimalString = "0" + decimalString;
  }

  return decimalString;
}
