#pragma once
#define min // workaround: nimble's min/max macros conflict with libstdc++
#define max
#include <host/ble_gap.h>
#include <atomic>
#undef max
#undef min

namespace Pinetime {
  namespace System {
    class SystemTask;
  }
  namespace Controllers {
    class SleepController;
    class SleepService {
    public:
      SleepService(Pinetime::System::SystemTask& system, Controllers::SleepController& sleepController);
      void Init();
      int OnSleepRequested(uint16_t connectionHandle, uint16_t attributeHandle, ble_gatt_access_ctxt* context);
      void OnNewSleepState();

      void SubscribeNotification(uint16_t connectionHandle, uint16_t attributeHandle);
      void UnsubscribeNotification(uint16_t connectionHandle, uint16_t attributeHandle);

    private:
      Pinetime::System::SystemTask& system;
      Controllers::SleepController& sleepController;

      struct ble_gatt_chr_def characteristicDefinition[4];
      struct ble_gatt_svc_def serviceDefinition[2];

      uint16_t sleepStateHandle;
      uint16_t sleepDurationHandle;
      uint16_t sleepQualityHandle;
      std::atomic_bool sleepStateNoficationEnabled {false};
    };
  }
}