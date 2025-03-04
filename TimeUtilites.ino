#include <time.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600 * 9); // UTC+9 (한국 시간)

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