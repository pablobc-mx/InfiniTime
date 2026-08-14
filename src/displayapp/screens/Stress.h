#pragma once

#include <cstdint>
#include <lvgl/lvgl.h>
#include "displayapp/screens/Screen.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "systemtask/SystemTask.h"
#include "systemtask/WakeLock.h"
#include "Symbols.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {
      class Stress : public Screen {
      public:
        Stress(Controllers::HeartRateController& heartRateController,
               Controllers::MotionController& motionController,
               Controllers::Settings& settingsController,
               System::SystemTask& systemTask);
        ~Stress() override;
        void Refresh() override;
        void UpdateSelected(lv_obj_t* object, lv_event_t event);

      private:
        static constexpr size_t maxSamples = 30;
        static constexpr size_t minSamples = 20;
        // Motion gate: sum of |axis deltas| above this counts as "moving".
        // Raised from 25: a wrist at rest still shows small axis deltas
        // (breathing, pulse, micro-shifts) that exceeded the old threshold.
        static constexpr int motionThreshold = 60;
        // Only wipe the sample buffer after this many CONSECUTIVE moving
        // refreshes (~1s each) — brief movements shouldn't destroy progress.
        static constexpr int sustainedMotionTicks = 3;

        Controllers::HeartRateController& heartRateController;
        Controllers::MotionController& motionController;
        Controllers::Settings& settingsController;
        Pinetime::System::WakeLock wakeLock;

        uint8_t samples[maxSamples] = {0};
        size_t sampleCount = 0;
        int movingTicks = 0;
        int16_t prevX = 0;
        int16_t prevY = 0;
        int16_t prevZ = 0;

        uint8_t stressScore = 0;
        bool hasScore = false;

        void ComputeScore();
        void UpdateUI();

        lv_obj_t* labelScore;
        lv_obj_t* labelStatus;
        lv_obj_t* labelHr;
        lv_obj_t* labelResting;
        lv_obj_t* btnPlus;
        lv_obj_t* btnMinus;
        lv_task_t* taskRefresh;
      };
    }

    template <>
    struct AppTraits<Apps::Stress> {
      static constexpr Apps app = Apps::Stress;
      static constexpr const char* icon = Screens::Symbols::heartBeat;

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::Stress(controllers.heartRateController,
                                   controllers.motionController,
                                   controllers.settingsController,
                                   *controllers.systemTask);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      };
    };
  }
}
