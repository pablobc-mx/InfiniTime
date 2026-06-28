#include "drivers/SC7A20.h"
#include "drivers/SC7A20_registers.h"
#include <libraries/delay/nrf_delay.h>
#include <task.h>
#include <cmath>

using namespace Pinetime::Drivers;

SC7A20::SC7A20(TwiMaster& twiMaster, uint8_t twiAddress) : twiMaster{twiMaster}, deviceAddress{twiAddress} {
}

void SC7A20::Init() {
  // Reset internal memory
  uint8_t data = CTRL_REG5_BOOT;
  Write(CTRL_REG5, &data, 1);
  vTaskDelay(5);
  data = 0;
  Write(CTRL_REG5, &data, 1);

  // Read Chip ID
  Read(WHO_AM_I, &data, 1);
  if (data == 17) {
    deviceType = 1; // SC7A20
  } else {
    deviceType = 0; // Unknown
    return;
  }

  // ±2g range
  data = CTRL_REG4_FS_2G;
  Write(CTRL_REG4, &data, 1);

  // Block update + high-resolution mode (12 bit)
  data = CTRL_REG4_BDU | CTRL_REG4_HR;
  Write(CTRL_REG4, &data, 1);

  // FIFO enabled, FIFO mode
  data = CTRL_REG5_FIFO_EN;
  Write(CTRL_REG5, &data, 1);
  data = FIFO_CTRL_REG_FIFO;
  Write(FIFO_CTRL_REG, &data, 1);

  // 200 Hz, all axes enabled
  data = CTRL_REG1_ODR_200HZ | CTRL_REG1_X_EN | CTRL_REG1_Y_EN | CTRL_REG1_Z_EN;
  Write(CTRL_REG1, &data, 1);

  isOk = true;
}

void SC7A20::SoftReset() {
  uint8_t data = CTRL_REG5_BOOT;
  Write(CTRL_REG5, &data, 1);
  vTaskDelay(5);
  data = 0;
  Write(CTRL_REG5, &data, 1);
}

SC7A20::Values SC7A20::Process() {
  if (!isOk)
    return {};

  // Read FIFO size (~20 samples = 200 Hz / 10 Hz main loop)
  uint8_t length = 0;
  Read(FIFO_SRC_REG, &length, 1);
  length &= FIFO_SRC_REG_FSS_MASK;

  // Read samples one by one
  for (uint8_t i = 0; i < length; i++) {
    Read(0x80 | OUT_X_L, (uint8_t*) &fifo[i], sizeof(int16_t) * 3);
    for (uint8_t j = 0; j < 3; j++)
      fifo[i][j] >>= (16 - 12); // right-justify 12-bit
    // X/Y swapped due to P8 mounting
    int16_t swap = fifo[i][0];
    fifo[i][0] = fifo[i][1];
    fifo[i][1] = swap;
  }

  // Restart FIFO
  uint8_t data = FIFO_CTRL_REG_BYPASS;
  Write(FIFO_CTRL_REG, &data, 1);
  data = FIFO_CTRL_REG_FIFO;
  Write(FIFO_CTRL_REG, &data, 1);

  // Averages in mg (±2g = 2000 mg range, 12-bit)
  int16_t avgs[3] = {0};
  if (length > 0) {
    for (uint8_t i = 0; i < length; i++)
      for (uint8_t j = 0; j < 3; j++)
        avgs[j] += (fifo[i][j] * 2000) / (1 << 11);
    for (uint8_t j = 0; j < 3; j++)
      avgs[j] /= length;
  }

  // --- Software step counting ---
  for (uint8_t i = 0; i < length; i++) {
    sampleCounter++;

    // Magnitude (FPU handles this efficiently)
    float mag = sqrtf(static_cast<float>(fifo[i][0] * fifo[i][0] +
                                          fifo[i][1] * fifo[i][1] +
                                          fifo[i][2] * fifo[i][2]));

    // Moving-average filter
    filterBuf[filterIdx] = static_cast<int32_t>(mag);
    filterIdx = (filterIdx + 1) % filterSize;
    int32_t sum = 0;
    for (uint8_t j = 0; j < filterSize; j++)
      sum += filterBuf[j];
    int32_t smoothed = sum / static_cast<int32_t>(filterSize);

    // Sliding window
    histSum -= hist[histIdx];
    histSumSq -= static_cast<int64_t>(hist[histIdx]) * hist[histIdx];

    hist[histIdx] = smoothed;
    histSum += smoothed;
    histSumSq += static_cast<int64_t>(smoothed) * smoothed;
    histIdx = (histIdx + 1) % lag;
    if (histCount < lag)
      histCount++;

    if (histCount >= lag) {
      int32_t mean = histSum / static_cast<int32_t>(lag);
      int64_t variance = (histSumSq / static_cast<int64_t>(lag)) -
                          static_cast<int64_t>(mean) * mean;
      int32_t stddev = 0;
      if (variance > 0)
        stddev = static_cast<int32_t>(sqrtf(static_cast<float>(variance)));
      if (stddev < minStddev)
        stddev = minStddev;

      int32_t peakLine = mean + (static_cast<int64_t>(stddev) * threshold / 1000);
      if (smoothed > peakLine &&
          (sampleCounter - lastPeakSample) >= minPeakDist) {
        stepCount++;
        lastPeakSample = sampleCounter;
      }
    }
  }

  return {stepCount, avgs[0], avgs[1], avgs[2]};
}

void SC7A20::ResetStepCounter() {
  stepCount = 0;
  sampleCounter = 0;
  lastPeakSample = -static_cast<int32_t>(minPeakDist);
  histCount = 0;
  histSum = 0;
  histSumSq = 0;
  histIdx = 0;
  for (uint8_t i = 0; i < lag; i++) hist[i] = 0;
  for (uint8_t i = 0; i < filterSize; i++) filterBuf[i] = 0;
  filterIdx = 0;
}

void SC7A20::Read(uint8_t registerAddress, uint8_t* buffer, size_t size) {
  twiMaster.Read(deviceAddress, registerAddress, buffer, size);
}

void SC7A20::Write(uint8_t registerAddress, const uint8_t* data, size_t size) {
  twiMaster.Write(deviceAddress, registerAddress, data, size);
}

bool SC7A20::IsOk() const {
  return isOk;
}

uint8_t SC7A20::DeviceType() const {
  return deviceType;
}