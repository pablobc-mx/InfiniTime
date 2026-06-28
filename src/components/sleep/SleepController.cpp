#include "components/sleep/SleepController.h"
#include "components/fs/FS.h"
#include <nrf_log.h>
#include <algorithm>
#include <cmath>
#include <littlefs/lfs.h>

using namespace Pinetime::Controllers;

SleepController::SleepController(FS& fs) : fs{fs} {
}

void SleepController::Update(int16_t x, int16_t y, int16_t z, uint32_t currentTimeSec) {
  // Input guard: if time hasn't been set yet (epoch 0), skip
  if (currentTimeSec < 100000) {
    lastTickTime = 0; // reset monotonic tracker until time is real
    return;
  }

  // Detect time jumps (clock sync, DST, manual set).
  // If time moves backward or jumps forward > 120 sec, reset timestamps
  // to avoid duration underflow / phantom sleep.
  if (lastTickTime != 0) {
    int32_t delta = static_cast<int32_t>(currentTimeSec - lastTickTime);
    if (delta < 0 || delta > 120) {
      stillSince = 0;
      motionSince = 0;
      lastRestlessCheck = 0;
      // Don't abort an in-progress session — just reset the motion timers
      // so the duration calcs recover on the next tick.
    }
  }
  lastTickTime = currentTimeSec;

  int16_t dx = std::abs(x - lastX);
  int16_t dy = std::abs(y - lastY);
  int16_t dz = std::abs(z - lastZ);
  int32_t movement = static_cast<int32_t>(dx) + dy + dz;
  lastX = x; lastY = y; lastZ = z;

  bool isStill = (movement < stillThreshold);

  switch (state) {
    case SleepState::Awake: {
      if (isStill) {
        if (stillSince == 0) stillSince = currentTimeSec;
        motionSince = 0;

        uint32_t stillDuration = currentTimeSec - stillSince;
        if (stillDuration >= idleSecToSleep) {
          state = SleepState::Asleep;
          sessionStartTime = currentTimeSec;
          sessionSeconds = 0;
          restlessCount = 0;
          lastRestlessCheck = currentTimeSec;
          NRF_LOG_INFO("[Sleep] Asleep at t=%u (still for %u sec)", currentTimeSec, stillDuration);
        }
      } else {
        stillSince = 0;
      }
      break;
    }

    case SleepState::Asleep: {
      sessionSeconds = currentTimeSec - sessionStartTime;

      if (!isStill) {
        // Movement during sleep
        if (motionSince == 0) motionSince = currentTimeSec;
        stillSince = 0;

        // Track restlessness in 2-min windows
        if (currentTimeSec - lastRestlessCheck >= restlessWindowSec) {
          restlessCount++;
          lastRestlessCheck = currentTimeSec;
        }

        uint32_t motionDuration = currentTimeSec - motionSince;
        if (motionDuration >= wakeSec) {
          // Wake up — save session
          uint32_t durationMin = sessionSeconds / 60;
          if (durationMin < 1) durationMin = 1;

          // Quality: penalize restlessness per hour
          float hours = sessionSeconds / 3600.0f;
          uint8_t quality = 100;
          if (hours > 0) {
            float rph = restlessCount / hours;
            quality = static_cast<uint8_t>(std::max(0.0f, 100.0f - rph * 10.0f));
          }

          lastSessionStart = sessionStartTime;
          lastSessionDurationMin = durationMin;
          lastSessionQuality = quality;

          NRF_LOG_INFO("[Sleep] Wake: %u min, %u restless, quality=%u",
                         durationMin, restlessCount, quality);

          WriteStatusFile();

          state = SleepState::Awake;
          stillSince = 0;
          motionSince = 0;
        }
      } else {
        motionSince = 0;
      }
      break;
    }
  }
}

void SleepController::WriteStatusFile() {
  if (lastSessionDurationMin == 0) return;

  lfs_file_t file;
  int res = fs.FileOpen(&file, "/SLEEP.BIN", LFS_O_RDWR | LFS_O_CREAT);
  if (res != 0) return;

  fs.FileSeek(&file, 0);
  lfs_info info;
  if (fs.Stat("/SLEEP.BIN", &info) == 0) {
    fs.FileSeek(&file, info.size);
  }

  struct __attribute__((packed)) {
    uint32_t startTs;
    uint16_t durationMin;
    uint8_t quality;
  } record;

  record.startTs = lastSessionStart;
  record.durationMin = lastSessionDurationMin;
  record.quality = lastSessionQuality;

  fs.FileWrite(&file, reinterpret_cast<const uint8_t*>(&record), sizeof(record));
  fs.FileClose(&file);

  NRF_LOG_INFO("[Sleep] Saved session to /SLEEP.BIN");
}