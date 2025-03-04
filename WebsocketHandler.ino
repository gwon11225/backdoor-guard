#include <WebSocketsClient.h>
#include <ArduinoJson.h>

const char* serverAddress = "backdoor.baekjoon.kr"; // 서버 주소
const int port = 443;                               // HTTPS 포트
const char* socketPath = "/ws";                     // 웹 소켓 경로

JsonDocument doc;
WebSocketsClient webSocket;
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);

void webSocketStart() {
  webSocket.beginSSL(serverAddress, port, socketPath);
  webSocket.setReconnectInterval(1000);
  webSocket.onEvent(webSocketEvent);
}

void webSocketLoop() {
  webSocket.loop();
}

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