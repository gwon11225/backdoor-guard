#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_eap_client.h"

#define SS_PIN 5
#define RST_PIN 22

#define MAGNETIC_SENSOR 4
#define SOLENOID_MODULE 13

const char *ssid = "jiyong3044";
const char *password = "22112211";

bool isDoorOpen = false;
unsigned long doorOpenTime;
int counter = 0;
int outingScheduleNumber = 0;

void setup() {
  Serial.begin(115200);

  pinMode(MAGNETIC_SENSOR, INPUT_PULLUP);
  pinMode(SOLENOID_MODULE, OUTPUT);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWi-Fi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  rfidStart();
  webSocketStart();

  doorOpenTime = millis();

  isDoorOpen = false;
  digitalWrite(SOLENOID_MODULE, HIGH);
}

void loop() {
  webSocketLoop();
  unsigned long currentTime = millis();

  bool isContactMagnetic = digitalRead(MAGNETIC_SENSOR) == LOW;
  bool isCorrectTime = isGoingOutPossible();
  bool isThreeSecondAgo = currentTime - doorOpenTime > 3000;
  bool isVaildCard = isValidRFID();

  bool isOpen = isVaildCard && isCorrectTime;
  bool isClose = isThreeSecondAgo && isDoorOpen && isContactMagnetic;

  if (isOpen) {
    Serial.println("Door is Open");
    digitalWrite(SOLENOID_MODULE, LOW);
    isDoorOpen = true;
    doorOpenTime = millis();
  }

  if (isClose) {
    Serial.println("Door close");
    digitalWrite(SOLENOID_MODULE, HIGH);
    isDoorOpen = false;
  }
}



