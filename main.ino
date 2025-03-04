#include <WiFi.h>
#include <WebSocketsClient.h>
#include "esp_wifi.h"
#include "esp_eap_client.h"
#include <ArduinoJson.h>
#include <Vector.h>
#include <SPI.h>
#include <MFRC522.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>

#define SS_PIN 5
#define RST_PIN 22

#define MAGNETIC_SENSOR 4
#define SOLENOID_MODULE 13

typedef struct {
  String startTime;
  String endTime;
} OutingSchedule;

const char *ssid = "jiyong3044";
const char *password = "22112211";

const char* serverAddress = "backdoor.baekjoon.kr"; // 서버 주소
const int port = 443;                               // HTTPS 포트
const char* socketPath = "/ws";                     // 웹 소켓 경로

bool isDoorOpen = false;
unsigned long doorOpenTime;

int counter = 0;

WebSocketsClient webSocket;
OutingSchedule outingSchedules[50];
byte verifiedCardNuid[5][4] {
  {175, 120, 115, 55}
};
int outingScheduleNumber = 0;
JsonDocument doc;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600 * 9); // UTC+9 (한국 시간)
MFRC522 rfid(SS_PIN, RST_PIN);

byte nuidPICC[4];

// 함수 선언
bool parseISOTime(const String& isoTime, struct tm& timeinfo);
bool isTimeInRange(const String& startTimeStr, const String& endTimeStr);
bool isGoingOutPossible();
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
bool isCorrectCard(byte nuid[4]);

void setup() {
  Serial.begin(115200);

  pinMode(MAGNETIC_SENSOR, INPUT_PULLUP);
  pinMode(SOLENOID_MODULE, OUTPUT);

  SPI.begin();
  rfid.PCD_Init();

  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWi-Fi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  webSocket.beginSSL(serverAddress, port, socketPath);
  webSocket.setReconnectInterval(1000);
  webSocket.onEvent(webSocketEvent);

  doorOpenTime = millis();

  isDoorOpen = false;
  digitalWrite(SOLENOID_MODULE, HIGH);
}

void loop() {
  webSocket.loop();
  unsigned long currentTime = millis();

  bool isContactCard = rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial();
  bool isCorrectCardNuid = false;
  if (isContactCard) {
    nuidPICC[0] = rfid.uid.uidByte[0];
    nuidPICC[1] = rfid.uid.uidByte[1];
    nuidPICC[2] = rfid.uid.uidByte[2];
    nuidPICC[3] = rfid.uid.uidByte[3];

    isCorrectCardNuid = isCorrectCard(nuidPICC);
  }

  bool isContactMagnetic = digitalRead(MAGNETIC_SENSOR) == LOW;
  bool isCorrectTime = isGoingOutPossible();
  bool isThreeSecondAgo = currentTime - doorOpenTime > 3000;

  bool isOpen = isCorrectCardNuid && isCorrectTime;
  bool isClose = isThreeSecondAgo && isDoorOpen && isContactMagnetic;

  if (isContactMagnetic) {
    Serial.println("Magnetic Contact");
  }

  if (isContactCard) {
    Serial.println("Card Contact");
    Serial.print(rfid.uid.uidByte[0]);
    Serial.print(" ");
    Serial.print(rfid.uid.uidByte[1]);
    Serial.print(" ");
    Serial.print(rfid.uid.uidByte[2]);
    Serial.print(" ");
    Serial.print(rfid.uid.uidByte[3]);
    Serial.println();
  }

  if (isCorrectCardNuid) {
    Serial.println("Correct Card");
  }

  if (isCorrectTime) {
    Serial.println("Correct Time");
  }

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

// 웹 소켓 이벤트 핸들러
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("Disconnected from WebSocket server");
      break;

    case WStype_CONNECTED:
      Serial.println("Connected to WebSocket server");
      webSocket.sendTXT("Hello from ESP32!");  // 서버에 메시지 전송
      break;

    case WStype_TEXT:
      Serial.printf("Message from server: %s\n", payload);  // 서버로부터 수신한 메시지 출력
      deserializeJson(doc, payload);
      outingScheduleNumber = 0;
      for (int i = 0; i < doc.size(); i++) {
        JsonVariant outingSchedule = doc[i];
        String startTime = outingSchedule["outingStartTime"].as<String>();
        String endTime = outingSchedule["outingEndTime"].as<String>();

        Serial.print("start time: ");
        Serial.print(startTime);
        Serial.print(", end time: ");
        Serial.println(endTime);

        outingSchedules[i] = {startTime, endTime};
        outingScheduleNumber = i + 1;

        Serial.println();
      }
      break;

    default:
      break;
  }
}

bool isGoingOutPossible() {
  for (int outingScheduleIndex = 0; outingScheduleIndex < outingScheduleNumber; outingScheduleIndex++) {
    if (isTimeInRange(outingSchedules[outingScheduleIndex].startTime, outingSchedules[outingScheduleIndex].endTime)) {
      return true;
    }
  }
  return false;
}

bool parseISOTime(const String& isoTime, struct tm& timeinfo) {
  int year, month, day, hour, minute, second;
  if (sscanf(isoTime.c_str(), "%d-%d-%dT%d:%d:%d", 
             &year, &month, &day, &hour, &minute, &second) == 6) {
    timeinfo.tm_year = year - 1900;  // 연도는 1900년부터의 년수
    timeinfo.tm_mon = month - 1;     // 월은 0-11 범위
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;
    
    return true;
  }
  return false;
}

bool isTimeInRange(const String& startTimeStr, const String& endTimeStr) {
  struct tm startTime = {0};
  struct tm endTime = {0};
  struct tm currentTime = {0};

  // NTP 서버에서 현재 시간 가져오기
  timeClient.update();
  time_t now = timeClient.getEpochTime();
  localtime_r(&now, &currentTime);

  // ISO 시간 문자열 파싱
  if (!parseISOTime(startTimeStr, startTime) || 
      !parseISOTime(endTimeStr, endTime)) {
    Serial.println("시간 파싱 실패");
    return false;
  }

  // 시작 시간과 종료 시간의 epoch 시간 계산
  time_t startEpoch = mktime(&startTime);
  time_t endEpoch = mktime(&endTime);

  // 현재 시간이 시작 시간과 종료 시간 사이인지 확인
  return (now >= startEpoch && now <= endEpoch);
}

bool isCorrectCard(byte nuid[4]) {
  for (int cardIndex = 0; cardIndex < 5; cardIndex++) {
    if (
      verifiedCardNuid[cardIndex][0] == nuid[0] ||
      verifiedCardNuid[cardIndex][1] == nuid[1] ||
      verifiedCardNuid[cardIndex][2] == nuid[2] ||
      verifiedCardNuid[cardIndex][3] == nuid[3]
    ) {
      return true;
    }
  }
  return false;
}

