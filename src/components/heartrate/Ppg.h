#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include "components/heartrate/Biquad.h"
#include "components/heartrate/Ptagc.h"

namespace Pinetime {
  namespace Controllers {
    class Ppg {
    public:
      Ppg();
      int8_t Preprocess(float spl);
      float HeartRate();

      void SetOffset(uint16_t i);
      void Reset();

    private:
      static constexpr size_t dataLength = 150;
      static constexpr size_t overlapShift = 25;

      std::array<int8_t, dataLength> data;
      size_t dataIndex = 0;
      float offset;
      Biquad hpf;
      Ptagc agc;
      Biquad lpf;
      float runningAvg[5] = {0};
      uint8_t avgIdx = 0;
      uint8_t avgCount = 0;

      float ProcessHeartRate();
    };
  }
}