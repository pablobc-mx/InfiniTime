#pragma once

#include "displayapp/screens/Screen.h"
#include "displayapp/screens/Symbols.h"
#include "systemtask/SystemTask.h"
#include <cstdint>
#include <lvgl/lvgl.h>

namespace Pinetime {
  namespace Applications {
    namespace Screens {
      class FindPhone : public Screen {
      public:
        explicit FindPhone(System::SystemTask& systemTask, Controllers::MotorController& motorController);
        ~FindPhone() override;
        void Refresh() override;

      private:
        System::SystemTask& systemTask;
        Controllers::MotorController& motorController;
        lv_obj_t* btnRing;
        lv_obj_t* labelStatus;
        lv_obj_t* labelIcon;
        lv_task_t* taskRefresh;

        void OnRing();
        static void btnEventHandler(lv_obj_t* obj, lv_event_t event);
      };
    }

    template <>
    struct AppTraits<Apps::FindPhone> {
      static constexpr Apps app = Apps::FindPhone;
      static constexpr const char* icon = Screens::Symbols::phone;

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::FindPhone(*controllers.systemTask, controllers.motorController);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      };
    };
  }
}