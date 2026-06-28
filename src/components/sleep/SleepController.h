#pragma once

#include <cstdint>

namespace Pinetime {
  namespace Controllers {
    class FS;

    class SleepController {
    public:
      enum class SleepState : uint8_t { Awake = 0, Asleep = 1 };

      SleepController(FS& fs);

      /// Called from SystemTask::UpdateMotion() each tick (~10 Hz).
      void Update(int16_t x, int16_t y, int16_t z, uint32_t currentTimeSec);

      SleepState CurrentState() const { return state; }

      uint32_t LastSleepStart() const { return lastSessionStart; }
      uint32_t LastSleepDurationMin() const { return lastSessionDurationMin; }
      uint8_t  LastSleepQuality() const { return lastSessionQuality; }

      void WriteStatusFile();

    private:
      static constexpr int16_t stillThreshold = 30;     // max mag change to count as "still"
      static constexpr uint32_t idleSecToSleep = 900;    // 15 min of stillness → asleep
      static constexpr uint32_t wakeSec = 300;           // 5 min of motion → wake
      static constexpr uint32_t restlessWindowSec = 120; // 2 min window for restlessness

      FS& fs;
      SleepState state = SleepState::Awake;

      // Motion tracking (wall-clock seconds)
      int16_t lastX = 0, lastY = 0, lastZ = 0;
      uint32_t stillSince = 0;    // timestamp when stillness began
      uint32_t motionSince = 0;   // timestamp when motion began
      uint32_t lastRestlessCheck = 0;
      uint32_t restlessCount = 0;

      // Session tracking
      uint32_t sessionStartTime = 0;
      uint32_t sessionSeconds = 0;

      // Last completed session
      uint32_t lastSessionStart = 0;
      uint32_t lastSessionDurationMin = 0;
      uint8_t lastSessionQuality = 0;
    };
  }
}