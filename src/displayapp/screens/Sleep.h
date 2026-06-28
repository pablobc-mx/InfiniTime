#pragma once

#include <cstdint>
#include <lvgl/lvgl.h>
#include "displayapp/screens/Screen.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "components/sleep/SleepController.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {
      class Sleep : public Screen {
      public:
        Sleep(Controllers::SleepController& sleepController);
        ~Sleep() override;
        void Refresh() override;

      private:
        Controllers::SleepController& sleepController;
        lv_obj_t* lState;
        lv_obj_t* lDuration;
        lv_obj_t* lQuality;
        lv_obj_t* lStart;
        lv_task_t* taskRefresh;
      };
    }

    template <>
    struct AppTraits<Apps::Sleep> {
      static constexpr Apps app = Apps::Sleep;
      static constexpr const char* icon = Screens::Symbols::sleep;

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::Sleep(*controllers.sleepController);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      };
    };
  }
}