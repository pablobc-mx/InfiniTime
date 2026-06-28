/*
  SPDX-License-Identifier: LGPL-3.0-or-later
  Original work Copyright (C) 2020 Daniel Thompson
  C++ port Copyright (C) 2021 Jean-François Milants
*/

#include "components/heartrate/Ppg.h"
#include <vector>
#include <nrf_log.h>

using namespace Pinetime::Controllers;

/** Original implementation from wasp-os : https://github.com/daniel-thompson/wasp-os/blob/master/wasp/ppg.py */
namespace {
  constexpr int troughRangeMin = 7;
  constexpr int troughRangeMax = 48;

  int Compare(int8_t* d1, int8_t* d2, size_t count) {
    int e = 0;
    for (size_t i = 0; i < count; i++) {
      auto d = d1[i] - d2[i];
      e += d * d;
    }
    return e;
  }

  int CompareShift(int8_t* d, int shift, size_t count) {
    if (shift < 0 || static_cast<size_t>(shift) >= count)
      return -1;
    return Compare(d + shift, d, count - shift);
  }

  int Trough(int8_t* d, size_t size, uint8_t mn, uint8_t mx) {
    // Clamp to available data: need 3 elements minimum for comparison
    if (static_cast<size_t>(mx) + 2 >= size)
      mx = (size > 3) ? size - 3 : 0;
    if (mn >= mx)
      return -1;

    auto z2 = CompareShift(d, mn - 2, size);
    if (z2 < 0) return -1;
    auto z1 = CompareShift(d, mn - 1, size);
    if (z1 < 0) return -1;
    for (int i = mn; i < mx + 1; i++) {
      auto z = CompareShift(d, i, size);
      if (z < 0) break;
      if (z2 > z1 && z1 < z)
        return i;
      z2 = z1;
      z1 = z;
    }
    return -1;
  }
}

Ppg::Ppg()
  : hpf {0.87033078, -1.74066156, 0.87033078, -1.72377617, 0.75754694},
    agc {20, 0.971, 2},
    lpf {0.11595249, 0.23190498, 0.11595249, -0.72168143, 0.18549138} {
}

int8_t Ppg::Preprocess(float spl) {
  spl -= offset;
  spl = hpf.Step(spl);
  spl = agc.Step(spl);
  spl = lpf.Step(spl);

  auto spl_int = static_cast<int8_t>(spl);

  if (dataIndex < dataLength)
    data[dataIndex++] = spl_int;
  return spl_int;
}

float Ppg::HeartRate() {
  if (dataIndex < dataLength)
    return 0;

  float hr = ProcessHeartRate();

  // Shift buffer: keep dataLength - overlapShift samples, discard oldest
  size_t keep = dataLength - overlapShift;
  for (size_t i = 0; i < keep; i++)
    data[i] = data[i + overlapShift];
  dataIndex = keep;

  // Running average (5 values)
  if (hr > 0) {
    runningAvg[avgIdx] = hr;
    avgIdx = (avgIdx + 1) % 5;
    if (avgCount < 5)
      avgCount++;
  }

  if (avgCount > 0) {
    float sum = 0;
    for (uint8_t i = 0; i < avgCount; i++)
      sum += runningAvg[i];
    return sum / avgCount;
  }

  return 0;
}

float Ppg::ProcessHeartRate() {
  auto t0 = Trough(data.data(), dataIndex, troughRangeMin, troughRangeMax);
  if (t0 < 0)
    return 0;

  float t1 = t0 * 2;
  t1 = Trough(data.data(), dataIndex,
              static_cast<uint8_t>(t1 - 5), static_cast<uint8_t>(t1 + 5));
  if (t1 < 0)
    return 0;

  float t2 = static_cast<int>(t1 * 3) / 2;
  t2 = Trough(data.data(), dataIndex,
              static_cast<uint8_t>(t2 - 5), static_cast<uint8_t>(t2 + 5));
  if (t2 < 0)
    return 0;

  float t3 = static_cast<int>(t2 * 4) / 3;
  t3 = Trough(data.data(), dataIndex,
              static_cast<uint8_t>(t3 - 4), static_cast<uint8_t>(t3 + 4));

  if (t3 < 0)
    return static_cast<float>(60 * 24 * 3) / t2;

  return static_cast<float>(60 * 24 * 4) / t3;
}

void Ppg::SetOffset(uint16_t offset) {
  this->offset = offset;
  dataIndex = 0;
}

void Ppg::Reset() {
  dataIndex = 0;
  avgCount = 0;
  avgIdx = 0;
}