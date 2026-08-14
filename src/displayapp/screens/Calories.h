#pragma once

#include <cstdint>
#include <lvgl/lvgl.h>
#include "displayapp/screens/Screen.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "Symbols.h"

namespace Pinetime {
  namespace Applications {
    namespace Screens {
      class Calories : public Screen {
      public:
        Calories(Controllers::Settings& settingsController,
                 Controllers::MotionController& motionController,
                 Controllers::DateTime& dateTimeController);
        ~Calories() override;
        void Refresh() override;
        void UpdateSelected(lv_obj_t* object, lv_event_t event);

      private:
        Controllers::Settings& settingsController;
        Controllers::MotionController& motionController;
        Controllers::DateTime& dateTimeController;

        uint16_t CalculateBmr() const;
        void RefreshKcal();

        lv_obj_t* labelKcal;
        lv_obj_t* labelSteps;
        lv_obj_t* labelWeight;
        lv_obj_t* btnPlus;
        lv_obj_t* btnMinus;
        lv_task_t* taskRefresh;
      };
    }

    template <>
    struct AppTraits<Apps::Calories> {
      static constexpr Apps app = Apps::Calories;
      static constexpr const char* icon = Screens::Symbols::tachometer;

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::Calories(controllers.settingsController, controllers.motionController, controllers.dateTimeController);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      };
    };
  }
}
