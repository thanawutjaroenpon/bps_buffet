#include <SPI.h>
#include <MFRC522.h>

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

#define SS_PIN D4
#define RST_PIN D3

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

void setup() {

  SPI.begin();      // Init SPI bus
  rfid.PCD_Init();  // Init MFRC522

  Serial.begin(9600);
  while (!Serial) {
    ;
  }
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

    // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
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
