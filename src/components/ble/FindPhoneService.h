#pragma once
#define min // workaround: nimble's min/max macros conflict with libstdc++
#define max
#include <host/ble_gap.h>
#include <atomic>
#undef max
#undef min

namespace Pinetime {
  namespace Controllers {
    class NimbleController;

    class FindPhoneService {
    public:
      explicit FindPhoneService(NimbleController& nimble);
      void Init();
      void Ring();
      int OnWrite(uint16_t attributeHandle, ble_gatt_access_ctxt* context);

      void SubscribeNotification(uint16_t attributeHandle);
      void UnsubscribeNotification(uint16_t attributeHandle);

    private:
      NimbleController& nimble;

      struct ble_gatt_chr_def characteristicDefinition[2];
      struct ble_gatt_svc_def serviceDefinition[2];

      uint16_t ringHandle;
      std::atomic_bool ringNotificationEnabled {false};
    };
  }
}
