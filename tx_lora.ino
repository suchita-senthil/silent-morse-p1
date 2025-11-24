#include <Arduino.h>
#include <SoftwareSerial.h>

// NodeMCU pin mapping
#define LORA_RX 12
#define LORA_TX 14
#define BUTTON_HI 5
#define BUTTON_BYE 4

#define USB_BAUD 115200
#define LORA_BAUD 9600   // default AT baud for LoRa-E5

SoftwareSerial loraSerial(LORA_RX, LORA_TX); 

unsigned long debounce = 150;
unsigned long lastHi = 0, lastBye = 0;

void sendCommand(String cmd) {
  loraSerial.print(cmd);
  loraSerial.print("\r\n");
  Serial.print("-> ");
  Serial.println(cmd);
}


void sendMessage(String msg) {
  sendCommand("AT+TEST=TXLRSTR,\"" + msg + "\"");
}

void setup() {
  Serial.begin(USB_BAUD);
  loraSerial.begin(LORA_BAUD);

  pinMode(BUTTON_HI, INPUT_PULLUP);
  pinMode(BUTTON_BYE, INPUT_PULLUP);

  Serial.println("\n--- LoRa-E5 P2P Transmitter (TX Only) ---");


  sendCommand("AT+MODE=TEST");
  delay(200);
  sendCommand("AT+TEST=RFCFG,868,SF12,125,12,15,14,ON,OFF,OFF");
  delay(200);

  Serial.println("LoRa-E5 configured for TX on 868MHz.");
}

void loop() {

  // button 1 send . 
  if (digitalRead(BUTTON_HI) == LOW && millis() - lastHi > debounce) {
    sendMessage(".");
    lastHi = millis();
    while (digitalRead(BUTTON_HI) == LOW) delay(5);
  }

  // button 2 send _
  if (digitalRead(BUTTON_BYE) == LOW && millis() - lastBye > debounce) {
    sendMessage("_");
    lastBye = millis();
    while (digitalRead(BUTTON_BYE) == LOW) delay(5);
  }
}