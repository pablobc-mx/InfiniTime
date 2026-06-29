#pragma once
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <components/heartrate/Ppg.h>

#ifdef DRIVER_ACC_SC7A20

namespace Pinetime {
  namespace Drivers {
    class Hrs3300;
  }
  namespace Controllers {
    class HeartRateController;
  }
  namespace Applications {
    class HeartRateTask {
    public:
      enum class Messages : uint8_t { GoToSleep, WakeUp, StartMeasurement, StopMeasurement };
      enum class States { Idle, Running };

      explicit HeartRateTask(Drivers::Hrs3300& heartRateSensor, Controllers::HeartRateController& controller);
      void Start();
      void Work();
      void PushMessage(Messages msg);

    private:
      static void Process(void* instance);
      void StartMeasurement();
      void StopMeasurement();

      TaskHandle_t taskHandle;
      QueueHandle_t messageQueue;
      States state = States::Running;
      Drivers::Hrs3300& heartRateSensor;
      Controllers::HeartRateController& controller;
      Controllers::Ppg ppg;
      bool measurementStarted = false;
    };

  }
}

#else

#include <cstdint>
#include <optional>
#include "components/settings/Settings.h"

namespace Pinetime {
  namespace Drivers {
    class Hrs3300;
  }

  namespace Controllers {
    class HeartRateController;
  }

  namespace Applications {
    class HeartRateTask {
    public:
      enum class Messages : uint8_t { GoToSleep, WakeUp, Enable, Disable };

      explicit HeartRateTask(Drivers::Hrs3300& heartRateSensor,
                             Controllers::HeartRateController& controller,
                             Controllers::Settings& settings);
      void Start();
      void Work();
      void PushMessage(Messages msg);

    private:
      enum class States : uint8_t { Disabled, Waiting, BackgroundMeasuring, ForegroundMeasuring };
      static void Process(void* instance);
      void HandleSensorData();
      void StartMeasurement();
      void StopMeasurement();

      [[nodiscard]] bool BackgroundMeasurementNeeded() const;
      [[nodiscard]] std::optional<TickType_t> BackgroundMeasurementInterval() const;
      TickType_t CurrentTaskDelay();

      TaskHandle_t taskHandle;
      QueueHandle_t messageQueue;
      bool valueCurrentlyShown;
      bool measurementSucceeded;
      States state = States::Disabled;
      uint16_t count;
      Drivers::Hrs3300& heartRateSensor;
      Controllers::HeartRateController& controller;
      Controllers::Settings& settings;
      Controllers::Ppg ppg;
      TickType_t lastMeasurementTime;
      TickType_t measurementStartTime;
    };

  }
}

#endif
