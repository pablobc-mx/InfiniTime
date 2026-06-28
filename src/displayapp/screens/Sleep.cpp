#include "displayapp/screens/Sleep.h"
#include <lvgl/lvgl.h>
#include "displayapp/DisplayApp.h"

using namespace Pinetime::Applications::Screens;

Sleep::Sleep(Controllers::SleepController& sleepController)
  : sleepController{sleepController} {

  lState = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(lState, sleepController.CurrentState() == Controllers::SleepController::SleepState::Asleep
                                     ? "Sleeping..."
                                     : "Awake");
  lv_obj_set_style_local_text_color(lState, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_LIME);
  lv_obj_set_style_local_text_font(lState, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_42);
  lv_obj_align(lState, nullptr, LV_ALIGN_IN_TOP_MID, 0, 10);

  lDuration = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_fmt(lDuration, "Last: %lu min",
                         sleepController.LastSleepDurationMin());
  lv_obj_set_style_local_text_color(lDuration, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_CYAN);
  lv_obj_align(lDuration, nullptr, LV_ALIGN_CENTER, 0, -20);

  lQuality = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_fmt(lQuality, "Quality: %u%%",
                         sleepController.LastSleepQuality());
  lv_obj_align(lQuality, nullptr, LV_ALIGN_CENTER, 0, 10);

  lStart = lv_label_create(lv_scr_act(), nullptr);
  if (sleepController.LastSleepStart() > 0) {
    lv_label_set_text_fmt(lStart, "Start: %02u:%02u",
                           (sleepController.LastSleepStart() % 86400) / 3600,
                           (sleepController.LastSleepStart() % 3600) / 60);
  } else {
    lv_label_set_text_static(lStart, "Start: --:--");
  }
  lv_obj_align(lStart, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, -30);

  taskRefresh = lv_task_create(RefreshTaskCallback, 1000, LV_TASK_PRIO_MID, this);
}

Sleep::~Sleep() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void Sleep::Refresh() {
  auto state = sleepController.CurrentState();
  lv_label_set_text_static(lState, state == Controllers::SleepController::SleepState::Asleep
                                     ? "Sleeping..."
                                     : "Awake");

  lv_label_set_text_fmt(lDuration, "Last: %lu min",
                         sleepController.LastSleepDurationMin());
  lv_label_set_text_fmt(lQuality, "Quality: %u%%",
                         sleepController.LastSleepQuality());

  if (sleepController.LastSleepStart() > 0) {
    lv_label_set_text_fmt(lStart, "Start: %02u:%02u",
                           (sleepController.LastSleepStart() % 86400) / 3600,
                           (sleepController.LastSleepStart() % 3600) / 60);
  }
}