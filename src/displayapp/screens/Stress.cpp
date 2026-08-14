#include "displayapp/screens/Stress.h"
#include <cstdlib>
#include <lvgl/lvgl.h>
#include <components/heartrate/HeartRateController.h>
#include "components/settings/Settings.h"
#include "components/motion/MotionController.h"
#include "displayapp/DisplayApp.h"
#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;

namespace {
  void event_handler(lv_obj_t* obj, lv_event_t event) {
    Stress* screen = static_cast<Stress*>(obj->user_data);
    screen->UpdateSelected(obj, event);
  }
}

Stress::Stress(Controllers::HeartRateController& heartRateController,
               Controllers::MotionController& motionController,
               Controllers::Settings& settingsController,
               System::SystemTask& systemTask)
  : heartRateController {heartRateController},
    motionController {motionController},
    settingsController {settingsController},
    wakeLock(systemTask) {

  // Start HR measurement while this app is open (mirrors HeartRate screen's enable path)
  heartRateController.Enable();
  wakeLock.Lock();

  prevX = motionController.X();
  prevY = motionController.Y();
  prevZ = motionController.Z();

  lv_obj_t* title = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(title, "Stress");
  lv_label_set_align(title, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(title, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 15, 15);

  lv_obj_t* icon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(icon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_ORANGE);
  lv_label_set_text_static(icon, Symbols::heartBeat);
  lv_label_set_align(icon, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(icon, title, LV_ALIGN_OUT_LEFT_MID, -10, 0);

  // HR and resting baseline readouts — top row, clear of the bottom buttons
  labelHr = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(labelHr, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::lightGray);
  lv_obj_align(labelHr, lv_scr_act(), LV_ALIGN_IN_TOP_LEFT, 15, 55);

  labelResting = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(labelResting, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::lightGray);
  lv_obj_align(labelResting, lv_scr_act(), LV_ALIGN_IN_TOP_RIGHT, -15, 55);

  // Big stress score (0-100) — center, below the top row (y≈82-158)
  labelScore = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(labelScore, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_76);
  lv_obj_set_style_local_text_color(labelScore, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::lightGray);
  lv_label_set_text_static(labelScore, "--");
  lv_label_set_align(labelScore, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(labelScore, lv_scr_act(), LV_ALIGN_CENTER, 0, -5);

  labelStatus = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(labelStatus, "Measuring...");
  lv_label_set_align(labelStatus, LV_LABEL_ALIGN_CENTER);
  lv_obj_set_style_local_text_color(labelStatus, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::lightGray);
  lv_obj_align(labelStatus, labelScore, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

  // +/- buttons adjust the resting HR baseline (persisted via settings)
  static constexpr uint8_t btnWidth = 100;
  static constexpr uint8_t btnHeight = 45;

  btnPlus = lv_btn_create(lv_scr_act(), nullptr);
  btnPlus->user_data = this;
  lv_obj_set_size(btnPlus, btnWidth, btnHeight);
  lv_obj_align(btnPlus, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
  lv_obj_set_style_local_bg_color(btnPlus, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, Colors::bgAlt);
  lv_obj_set_event_cb(btnPlus, event_handler);
  lv_obj_t* lblPlus = lv_label_create(btnPlus, nullptr);
  lv_obj_set_style_local_text_font(lblPlus, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_42);
  lv_label_set_text_static(lblPlus, "+");

  btnMinus = lv_btn_create(lv_scr_act(), nullptr);
  btnMinus->user_data = this;
  lv_obj_set_size(btnMinus, btnWidth, btnHeight);
  lv_obj_align(btnMinus, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
  lv_obj_set_style_local_bg_color(btnMinus, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, Colors::bgAlt);
  lv_obj_set_event_cb(btnMinus, event_handler);
  lv_obj_t* lblMinus = lv_label_create(btnMinus, nullptr);
  lv_obj_set_style_local_text_font(lblMinus, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_42);
  lv_label_set_text_static(lblMinus, "-");

  UpdateUI();

  taskRefresh = lv_task_create(RefreshTaskCallback, 1000, LV_TASK_PRIO_LOW, this);
}

Stress::~Stress() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
  // Stop HR measurement when the app closes (mirrors HeartRate screen stop path)
  heartRateController.Disable();
  wakeLock.Release();
  settingsController.SaveSettings();
}

void Stress::ComputeScore() {
  // Heuristic stress index v1 (not medically validated):
  // score = clamp(round(elevation*2 + jitter*3), 0, 100)
  //   elevation = current BPM above resting baseline (>= 0)
  //   jitter    = mean |bpm[i] - bpm[i-1]| over collected samples (short-term variability)
  // Weights are arbitrary; tuned so a calm state (HR ~ resting, steady) scores low
  // and a raised + erratic HR scores high.
  uint32_t jitterSum = 0;
  for (size_t i = 1; i < sampleCount; i++) {
    uint8_t diff = samples[i] > samples[i - 1] ? samples[i] - samples[i - 1] : samples[i - 1] - samples[i];
    jitterSum += diff;
  }
  float jitter = static_cast<float>(jitterSum) / static_cast<float>(sampleCount - 1);

  uint16_t restingHr = settingsController.GetRestingHr();
  int16_t elevation = static_cast<int16_t>(samples[sampleCount - 1]) - static_cast<int16_t>(restingHr);
  if (elevation < 0) {
    elevation = 0;
  }

  float score = static_cast<float>(elevation) * 2.0f + jitter * 3.0f;
  if (score < 0.0f) {
    score = 0.0f;
  }
  if (score > 100.0f) {
    score = 100.0f;
  }
  stressScore = static_cast<uint8_t>(score + 0.5f); // round
  hasScore = true;
}

void Stress::UpdateUI() {
  lv_label_set_text_fmt(labelHr, "HR %u", heartRateController.HeartRate());
  lv_obj_realign(labelHr);

  lv_label_set_text_fmt(labelResting, "Rest %u", settingsController.GetRestingHr());
  lv_obj_realign(labelResting);

  if (!hasScore) {
    lv_label_set_text_static(labelScore, "--");
    lv_obj_set_style_local_text_color(labelScore, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::lightGray);
    return;
  }

  lv_label_set_text_fmt(labelScore, "%u", stressScore);
  lv_obj_realign(labelScore);

  lv_color_t color;
  const char* status;
  if (stressScore < 40) {
    color = LV_COLOR_GREEN;
    status = "Calm";
  } else if (stressScore < 70) {
    color = LV_COLOR_YELLOW;
    status = "Moderate";
  } else {
    color = LV_COLOR_RED;
    status = "High";
  }
  lv_obj_set_style_local_text_color(labelScore, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color);
  lv_label_set_text_static(labelStatus, status);
  lv_obj_set_style_local_text_color(labelStatus, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color);
}

void Stress::UpdateSelected(lv_obj_t* object, lv_event_t event) {
  uint16_t value = settingsController.GetRestingHr();

  int valueChange = 0;
  if (event == LV_EVENT_SHORT_CLICKED) {
    valueChange = 1;
  } else if (event == LV_EVENT_LONG_PRESSED || event == LV_EVENT_LONG_PRESSED_REPEAT) {
    valueChange = 5;
  } else {
    return;
  }

  if (object == btnPlus) {
    value += valueChange;
  } else if (object == btnMinus) {
    value -= valueChange;
  }

  if (value >= 40 && value <= 120) {
    settingsController.SetRestingHr(value);
    UpdateUI();
  }
}

void Stress::Refresh() {
  // Motion gating: HR from a moving wrist is not trustworthy. Instead of
  // wiping on a single movement tick (too sensitive — breathing/pulse
  // micro-shifts exceed it), only discard collected samples after several
  // CONSECUTIVE moving refreshes (~1s each).
  int16_t x = motionController.X();
  int16_t y = motionController.Y();
  int16_t z = motionController.Z();
  // Compute in int to avoid int16_t overflow when an axis delta is large
  int movement = std::abs(static_cast<int>(x) - static_cast<int>(prevX)) +
                 std::abs(static_cast<int>(y) - static_cast<int>(prevY)) +
                 std::abs(static_cast<int>(z) - static_cast<int>(prevZ));
  prevX = x;
  prevY = y;
  prevZ = z;

  if (movement > motionThreshold) {
    movingTicks++;
    if (movingTicks >= sustainedMotionTicks) {
      sampleCount = 0;
      hasScore = false;
    }
    lv_label_set_text_static(labelStatus, "Hold still...");
    lv_obj_set_style_local_text_color(labelStatus, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::lightGray);
    UpdateUI();
    return;
  }
  movingTicks = 0;

  uint8_t bpm = heartRateController.HeartRate();
  if (bpm == 0) {
    // No valid reading yet - keep waiting
    lv_label_set_text_fmt(labelStatus, "Measuring... (%d/%d)", sampleCount, minSamples);
    UpdateUI();
    return;
  }

  // Keep a sliding window of the last maxSamples readings
  if (sampleCount >= maxSamples) {
    for (size_t i = 1; i < maxSamples; i++) {
      samples[i - 1] = samples[i];
    }
    sampleCount = maxSamples - 1;
  }
  samples[sampleCount++] = bpm;

  if (sampleCount >= minSamples) {
    ComputeScore();
  } else {
    lv_label_set_text_fmt(labelStatus, "Measuring... (%d/%d)", sampleCount, minSamples);
  }
  UpdateUI();
}
