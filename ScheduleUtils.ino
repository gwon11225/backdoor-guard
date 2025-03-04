
typedef struct {
  String startTime;
  String endTime;
} OutingSchedule;

OutingSchedule outingSchedules[50];

bool isGoingOutPossible() {
  for (int outingScheduleIndex = 0; outingScheduleIndex < outingScheduleNumber; outingScheduleIndex++) {
    if (isTimeInRange(outingSchedules[outingScheduleIndex].startTime, outingSchedules[outingScheduleIndex].endTime)) {
      return true;
    }
  }
  return false;
}