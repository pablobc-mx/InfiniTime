#include "displayapp/screens/Calories.h"
#include <cmath>
#include <cstdlib>
#include <lvgl/lvgl.h>
#include "components/settings/Settings.h"
#include "components/motion/MotionController.h"
#include "components/datetime/DateTimeController.h"
#include "displayapp/DisplayApp.h"
#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;

namespace {
  void event_handler(lv_obj_t* obj, lv_event_t event) {
    Calories* screen = static_cast<Calories*>(obj->user_data);
    screen->UpdateSelected(obj, event);
  }
}

Calories::Calories(Controllers::Settings& settingsController,
                   Controllers::MotionController& motionController,
                   Controllers::DateTime& dateTimeController)
  : settingsController {settingsController}, motionController {motionController}, dateTimeController {dateTimeController} {

  lv_obj_t* title = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(title, "Calories");
  lv_label_set_align(title, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(title, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 15, 15);

  lv_obj_t* icon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(icon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_ORANGE);
  lv_label_set_text_static(icon, Symbols::tachometer);
  lv_label_set_align(icon, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(icon, title, LV_ALIGN_OUT_LEFT_MID, -10, 0);

  // Big daily kcal number
  labelKcal = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(labelKcal, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_42);
  lv_obj_set_style_local_text_color(labelKcal, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::highlight);
  lv_label_set_text_static(labelKcal, "--");
  lv_label_set_align(labelKcal, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(labelKcal, lv_scr_act(), LV_ALIGN_CENTER, 0, -40);

  lv_obj_t* labelKcalUnit = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(labelKcalUnit, "kcal today");
  lv_obj_set_style_local_text_color(labelKcalUnit, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::lightGray);
  lv_obj_align(labelKcalUnit, labelKcal, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

  // Steps and weight readouts — top row, clear of the bottom buttons
  labelSteps = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(labelSteps, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::lightGray);
  lv_obj_align(labelSteps, lv_scr_act(), LV_ALIGN_IN_TOP_LEFT, 15, 55);

  labelWeight = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(labelWeight, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::lightGray);
  lv_obj_align(labelWeight, lv_scr_act(), LV_ALIGN_IN_TOP_RIGHT, -15, 55);

  // +/- buttons adjust weight (persisted via settings)
  static constexpr uint8_t btnWidth = 100;
  static constexpr uint8_t btnHeight = 60;

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

  RefreshKcal();

  taskRefresh = lv_task_create(RefreshTaskCallback, 1000, LV_TASK_PRIO_LOW, this);
}

Calories::~Calories() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
  settingsController.SaveSettings();
}

uint16_t Calories::CalculateBmr() const {
  // Mifflin-St Jeor equation (male default):
  // BMR (kcal/day) = 10*weightKg + 6.25*heightCm - 5*age + 5
  float bmr = 10.0f * settingsController.GetWeightKg() + 6.25f * settingsController.GetHeightCm() -
              5.0f * settingsController.GetAge() + 5.0f;
  return static_cast<uint16_t>(bmr);
}

void Calories::RefreshKcal() {
  // Minutes elapsed since midnight -> baseline burn so far today
  uint32_t minutesElapsed = static_cast<uint32_t>(dateTimeController.Hours()) * 60 + dateTimeController.Minutes();
  float bmrPerMin = CalculateBmr() / 1440.0f;
  // Standard approximation: ~0.04 kcal per step
  float activityKcal = static_cast<float>(motionController.NbSteps()) * 0.04f;

  uint32_t kcal = static_cast<uint32_t>(std::lround(bmrPerMin * static_cast<float>(minutesElapsed) + activityKcal));
  lv_label_set_text_fmt(labelKcal, "%lu", kcal);
  lv_obj_realign(labelKcal);

  lv_label_set_text_fmt(labelSteps, "Steps %lu", motionController.NbSteps());
  lv_obj_realign(labelSteps);

  lv_label_set_text_fmt(labelWeight, "%u kg", settingsController.GetWeightKg());
  lv_obj_realign(labelWeight);
}

void Calories::UpdateSelected(lv_obj_t* object, lv_event_t event) {
  uint16_t value = settingsController.GetWeightKg();

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

  if (value >= 30 && value <= 200) {
    settingsController.SetWeightKg(value);
    RefreshKcal();
  }
}

void Calories::Refresh() {
  RefreshKcal();
}
