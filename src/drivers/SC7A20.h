#pragma once

#include <cstdint>
#include "drivers/TwiMaster.h"

namespace Pinetime {
  namespace Drivers {
    class SC7A20 {
    public:
      struct Values {
        uint32_t steps;
        int16_t x;
        int16_t y;
        int16_t z;
      };

      SC7A20(TwiMaster& twiMaster, uint8_t twiAddress);
      SC7A20(const SC7A20&) = delete;
      SC7A20& operator=(const SC7A20&) = delete;
      SC7A20(SC7A20&&) = delete;
      SC7A20& operator=(SC7A20&&) = delete;

      void Init();
      void SoftReset();
      Values Process();
      void ResetStepCounter();

      void Read(uint8_t registerAddress, uint8_t* buffer, size_t size);
      void Write(uint8_t registerAddress, const uint8_t* data, size_t size);

      bool IsOk() const;
      uint8_t DeviceType() const;

    private:
      TwiMaster& twiMaster;
      uint8_t deviceAddress = 0x18;
      bool isOk = false;
      uint8_t deviceType = 0;

      // Software step-counting state
      static constexpr uint8_t lag = 32;
      static constexpr uint8_t filterSize = 4;
      static constexpr uint16_t threshold = 3000; // 3.0 std-dev × 1000
      static constexpr int32_t minStddev = 50;
      static constexpr uint8_t minPeakDist = 20;  // 100 ms at 200 Hz

      int16_t fifo[32][3] = {0};
      int32_t filterBuf[4] = {0};
      uint8_t filterIdx = 0;

      int32_t hist[32] = {0};
      int32_t histSum = 0;
      int64_t histSumSq = 0;
      uint8_t histIdx = 0;
      uint8_t histCount = 0;

      int32_t lastPeakSample = -20;
      uint32_t sampleCounter = 0;
      uint32_t stepCount = 0;
    };
  }
}