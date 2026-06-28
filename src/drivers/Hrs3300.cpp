/*
  SPDX-License-Identifier: LGPL-3.0-or-later
  Original work Copyright (C) 2020 Daniel Thompson
  C++ port Copyright (C) 2021 Jean-François Milants
*/

#include "drivers/Hrs3300.h"
#include <algorithm>
#include <iterator>
#include <nrf_gpio.h>

#include <FreeRTOS.h>
#include <task.h>
#include <nrf_log.h>

using namespace Pinetime::Drivers;

namespace {
  static constexpr uint8_t ledDriveCurrentValue = 0x2f;
}

/** Driver for the HRS3300 heart rate sensor.
 * Original implementation from wasp-os : https://github.com/wasp-os/wasp-os/blob/master/wasp/drivers/hrs3300.py
 *
 * Experimentaly derived changes to improve signal/noise (see comments below) - Ceimour
 */
Hrs3300::Hrs3300(TwiMaster& twiMaster, uint8_t twiAddress) : twiMaster {twiMaster}, twiAddress {twiAddress} {
}

void Hrs3300::Init() {
  nrf_gpio_cfg_input(30, NRF_GPIO_PIN_NOPULL);

  Disable();
  vTaskDelay(100);

  // HRS disabled, 12.5 ms wait time between cycles, (partly) 20mA drive
  WriteRegister(static_cast<uint8_t>(Registers::Enable), 0x60);

  // (partly) 20mA drive, power on, low nibble 0xE for steady output
  WriteRegister(static_cast<uint8_t>(Registers::PDriver), 0x6E);

  // HRS and ALS both in 16-bit mode
  WriteRegister(static_cast<uint8_t>(Registers::Res), 0x88);

  // 8x gain for better readings on P8b hardware
  WriteRegister(static_cast<uint8_t>(Registers::Hgain), 0xc);
}

void Hrs3300::Enable() {
  NRF_LOG_INFO("ENABLE");
  auto value = ReadRegister(static_cast<uint8_t>(Registers::Enable));
  value |= 0x80;
  WriteRegister(static_cast<uint8_t>(Registers::Enable), value);

  WriteRegister(static_cast<uint8_t>(Registers::PDriver), ledDriveCurrentValue);
}

void Hrs3300::Disable() {
  NRF_LOG_INFO("DISABLE");
  auto value = ReadRegister(static_cast<uint8_t>(Registers::Enable));
  value &= ~0x80;
  WriteRegister(static_cast<uint8_t>(Registers::Enable), value);

  WriteRegister(static_cast<uint8_t>(Registers::PDriver), 0);
}

Hrs3300::PackedHrsAls Hrs3300::ReadHrsAls() {
  // Read registers individually (burst reads across non-contiguous 0x0b-0x0c gap
  // may produce garbage on some HRS3300 variants)
  auto hrsM = ReadRegister(static_cast<uint8_t>(Registers::C0DataM));
  auto hrsH = ReadRegister(static_cast<uint8_t>(Registers::C0DataH));
  auto hrsL = ReadRegister(static_cast<uint8_t>(Registers::C0dataL));
  auto alsM = ReadRegister(static_cast<uint8_t>(Registers::C1dataM));
  auto alsH = ReadRegister(static_cast<uint8_t>(Registers::C1dataH));
  auto alsL = ReadRegister(static_cast<uint8_t>(Registers::C1dataL));

  PackedHrsAls res;
  res.hrs = ((hrsL & 0x30) << 12) | (hrsM << 8) | ((hrsH & 0x0f) << 4) | (hrsL & 0x0f);
  res.als = ((alsH & 0x3f) << 11) | (alsM << 3) | (alsL & 0x07);
  return res;
}

void Hrs3300::WriteRegister(uint8_t reg, uint8_t data) {
  auto ret = twiMaster.Write(twiAddress, reg, &data, 1);
  if (ret != TwiMaster::ErrorCodes::NoError)
    NRF_LOG_INFO("WRITE ERROR");
}

uint8_t Hrs3300::ReadRegister(uint8_t reg) {
  uint8_t value;
  auto ret = twiMaster.Read(twiAddress, reg, &value, 1);
  if (ret != TwiMaster::ErrorCodes::NoError)
    NRF_LOG_INFO("READ ERROR");
  return value;
}
