#pragma once
#include "drivers/Cst816s.h"
#include "displayapp/TouchEvents.h"
#ifdef DRIVER_TOUCH_REPORT
#include <FreeRTOS.h>
#include <task.h>
#endif

namespace Pinetime {
  namespace Controllers {
    class TouchHandler {
    public:
      struct TouchPoint {
        int x;
        int y;
        bool touching;
      };

      bool ProcessTouchInfo(Drivers::Cst816S::TouchInfos info);

      bool IsTouching() const {
#ifdef DRIVER_TOUCH_REPORT
        return currentTouchPoint.touching ||
               (xTaskGetTickCount() - lastTouchTick < touchHoldTicks);
#else
        return currentTouchPoint.touching;
#endif
      }

      uint8_t GetX() const {
        return currentTouchPoint.x;
      }

      uint8_t GetY() const {
        return currentTouchPoint.y;
      }

      Pinetime::Applications::TouchEvents GestureGet();

    private:
#ifdef DRIVER_TOUCH_REPORT
      static constexpr TickType_t touchHoldTicks = pdMS_TO_TICKS(300);
#endif
      Pinetime::Applications::TouchEvents gesture;
      TouchPoint currentTouchPoint = {};
#ifdef DRIVER_TOUCH_REPORT
      TickType_t lastTouchTick = 0;
#endif
      bool gestureReleased = true;
    };
  }
}
