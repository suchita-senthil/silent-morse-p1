#include <Arduino.h>
#include <SoftwareSerial.h>

// NodeMCU pins
#define LORA_RX 12  // D6
#define LORA_TX 14  // D5

#define USB_BAUD 115200
#define LORA_BAUD 9600

SoftwareSerial lora(LORA_RX, LORA_TX);

// buffers and timing
String incoming = "";      // line assembler for LoRa output
String morseSeq = "";      // accumulated  dots and dashes
unsigned long lastSymbolMillis = 0;
const unsigned long IDLE_MS = 1500; // 1.5 seconds

void sendCommand(const char *cmd) {
  lora.print(cmd);
  lora.print("\r\n");
  Serial.print("-> ");
  Serial.println(cmd);
}

String translateMorse(const String &seq) {
  if (seq == ".-")   return "ATTACK";
  if (seq == "--")   return "ADVANCE";
  if (seq == "-.-")  return "HOLD";
  if (seq == "..")   return "COVER";
  if (seq == "...")  return "CEASE FIRE";
  return "UNKNOWN";
}

void setup() {
  Serial.begin(USB_BAUD);
  lora.begin(LORA_BAUD);

  Serial.println("\n--- LoRa-E5 Morse Receiver (Mapped) ---");

  sendCommand("AT+MODE=TEST");
  delay(500);

  sendCommand("AT+TEST=RFCFG,868,SF12,125,12,15,14,ON,OFF,OFF");
  delay(500);

  sendCommand("AT+TEST=RXLRPKT");
  delay(500);

  Serial.println("Ready. Waiting for 2E -> '.' and 5F -> '-'");
}

void loop() {
  while (lora.available()) {
    char c = (char)lora.read();
    if (c == '\r') continue; // ignore CR
    if (c == '\n') {
      incoming.trim();
      if (incoming.length() > 0) {
        if (incoming.startsWith("+TEST: RX")) {
          int q1 = incoming.indexOf('"');
          int q2 = incoming.lastIndexOf('"');
          String payload;
          if (q1 != -1 && q2 != -1 && q2 > q1) {
            payload = incoming.substring(q1 + 1, q2);
          } else {
            // fallback: maybe payload appears after comma
            int comma = incoming.indexOf(',');
            if (comma != -1 && comma + 1 < incoming.length()) {
              payload = incoming.substring(comma + 1);
              payload.trim();
            }
          }

          // Accept either literal hex string "2E"/"5F" or literal '.'/'-' if module prints that
          if (payload == "2E" || payload == ".") {
            morseSeq += ".";
            Serial.print("."); // immediate visual feedback
            lastSymbolMillis = millis();
          } else if (payload == "5F" || payload == "-") {
            morseSeq += "-";
            Serial.print("-"); // immediate visual feedback
            lastSymbolMillis = millis();
          } else {
            // Unknown payload; optional debug print
            Serial.print("[RX ");
            Serial.print(payload);
            Serial.println("]");
          }
        } else {
          // optionally print other lines (RSSI/SNR/OK/etc.)
          Serial.print("<- ");
          Serial.println(incoming);
        }
      }
      incoming = "";
    } else {
      incoming += c;
    }
  }

  // if idle for IDLE_MS and we have a sequence - submit
  if (morseSeq.length() > 0 && (millis() - lastSymbolMillis >= IDLE_MS)) {
    Serial.println(); // newline after the rolling '.' '-' feedback
    Serial.print("Sequence received: ");
    Serial.println(morseSeq);

    String translated = translateMorse(morseSeq);
    Serial.print("Translated -> ");
    Serial.println(translated);

    // clr for next seq
    morseSeq = "";
  }

  // AT commands to LoRa
  while (Serial.available()) {
    byte b = Serial.read();
    lora.write(b);
  }
  // Note: we don't read lora here because we already consumed lora in the top loop.
}