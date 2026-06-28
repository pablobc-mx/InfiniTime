#pragma once
#include "drivers/Cst816s.h"
#include "displayapp/TouchEvents.h"
#include <FreeRTOS.h>
#include <task.h>

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
        return currentTouchPoint.touching ||
               (xTaskGetTickCount() - lastTouchTick < touchHoldTicks);
      }

      uint8_t GetX() const {
        return currentTouchPoint.x;
      }

      uint8_t GetY() const {
        return currentTouchPoint.y;
      }

      Pinetime::Applications::TouchEvents GestureGet();

    private:
      static constexpr TickType_t touchHoldTicks = pdMS_TO_TICKS(300);
      Pinetime::Applications::TouchEvents gesture;
      TouchPoint currentTouchPoint = {};
      TickType_t lastTouchTick = 0;
      bool gestureReleased = true;
    };
  }
}
