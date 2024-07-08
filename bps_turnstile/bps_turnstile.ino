// Library
// https://www.arduino.cc/reference/en/libraries/ethernet/
// https://github.com/miguelbalboa/rfid
// http://arduino.esp8266.com/stable/package_esp8266com_index.json

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

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 109);
IPAddress dns(8, 8, 8, 8);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

IPAddress serverIP(185, 78, 166, 46);
int serverPort = 3133;

EthernetClient client;

MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  Ethernet.init(5);
  Ethernet.begin(mac, ip, dns, gateway, subnet);

  Serial.println();
  Serial.println(Ethernet.localIP());

  SPI.begin();      // Init SPI bus
  rfid.PCD_Init();  // Init MFRC522
}

void loop() {
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if (!rfid.PICC_ReadCardSerial())
    return;

  Serial.println(F("A new card has been detected."));

  String rfid_in = rfid_read();  // Read RFID data
  Serial.println(rfid_in);       // Print RFID data to serial monitor
  sendToAPI(rfid_in);

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
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
            digitalWrite(BUZZER, HIGH);
            delay(100);
            digitalWrite(BUZZER, LOW);
            delay(100);
            digitalWrite(BUZZER, HIGH);
            delay(100);
            digitalWrite(BUZZER, LOW);
            delay(700);
            digitalWrite(LED_PIN, LOW);
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
  } else {
    Serial.println("Connection failed");
  }
}

String rfid_read() {
  String content = "";

  // Concatenate UID parts
  for (int i = rfid.uid.size - 1; i >= 0; i--) {
    content.concat(String(rfid.uid.uidByte[i] < 0x10 ? "0" : ""));
    content.concat(String(rfid.uid.uidByte[i], HEX));
  }

  content.toUpperCase();  // Convert to uppercase

  Serial.println(content);

  unsigned long decimalValue = strtoul(content.c_str(), NULL, 16);
  String decimalString = String(decimalValue);

  while (decimalString.length() < 10) {
    decimalString = "0" + decimalString;
  }

  return decimalString;  // Return RFID UID as String
}