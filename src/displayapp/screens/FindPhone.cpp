#include "displayapp/screens/FindPhone.h"
#include "components/motor/MotorController.h"
#include "displayapp/DisplayApp.h"
#include <lvgl/lvgl.h>

using namespace Pinetime::Applications::Screens;

FindPhone::FindPhone(System::SystemTask& systemTask, Controllers::MotorController& motorController)
  : systemTask {systemTask}, motorController {motorController} {

  lv_obj_t* background = lv_cont_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_bg_opa(background, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_COVER);
  lv_obj_set_style_local_bg_color(background, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
  lv_obj_set_size(background, LV_HOR_RES, LV_VER_RES);
  lv_obj_set_pos(background, 0, 0);

  // Phone icon
  labelIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(labelIcon, Symbols::phone);
  lv_obj_set_style_local_text_font(labelIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_42);
  lv_obj_align(labelIcon, nullptr, LV_ALIGN_IN_TOP_MID, 0, 30);

  // Status text
  labelStatus = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(labelStatus, "Tap the button\nto ring your phone");
  lv_obj_set_style_local_text_color(labelStatus, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_obj_set_style_local_text_font(labelStatus, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_obj_align(labelStatus, nullptr, LV_ALIGN_CENTER, 0, -20);

  // Big ring button
  btnRing = lv_btn_create(lv_scr_act(), nullptr);
  lv_obj_set_size(btnRing, 160, 60);
  lv_obj_set_event_cb(btnRing, btnEventHandler);
  lv_obj_set_style_local_bg_color(btnRing, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
  lv_obj_set_style_local_bg_color(btnRing, LV_BTN_PART_MAIN, LV_STATE_PRESSED, LV_COLOR_OLIVE);
  btnRing->user_data = this;
  lv_obj_align(btnRing, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, -30);

  lv_obj_t* labelBtn = lv_label_create(btnRing, nullptr);
  lv_label_set_text_static(labelBtn, "🔔 Ring!");
  lv_obj_set_style_local_text_font(labelBtn, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_label_set_align(labelBtn, LV_ALIGN_CENTER);

  taskRefresh = lv_task_create(RefreshTaskCallback, 3000, LV_TASK_PRIO_LOW, this);
  lv_task_set_repeat_count(taskRefresh, 0); // one-shot, re-enabled after ring
}

FindPhone::~FindPhone() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void FindPhone::btnEventHandler(lv_obj_t* obj, lv_event_t event) {
  auto* screen = static_cast<FindPhone*>(obj->user_data);
  if (event == LV_EVENT_CLICKED) {
    screen->OnRing();
  }
}

void FindPhone::OnRing() {
  // Vibrate as confirmation
  motorController.RunForDuration(100);

  // Update UI
  lv_label_set_text_static(labelStatus, "🔔 Ringing phone...");
  lv_obj_set_style_local_bg_color(btnRing, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);

  // Re-enable the auto-revert timer
  lv_task_set_repeat_count(taskRefresh, 1);
  lv_task_reset(taskRefresh);
}

void FindPhone::Refresh() {
  // Reset to ready state
  lv_label_set_text_static(labelStatus, "Tap the button\nto ring your phone");
  lv_obj_set_style_local_bg_color(btnRing, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);

  // Task is one-shot, auto-disables after this
  lv_task_set_repeat_count(taskRefresh, 0);
}