#include "components/ble/SleepService.h"
#include "components/sleep/SleepController.h"
#include "systemtask/SystemTask.h"
#include <nrf_log.h>

using namespace Pinetime::Controllers;

namespace {
  // 0004yyxx-78fc-48fe-8e23-433b3a1942d0
  constexpr ble_uuid128_t CharUuid(uint8_t x, uint8_t y) {
    return ble_uuid128_t {.u = {.type = BLE_UUID_TYPE_128},
                          .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e, 0xfe, 0x48, 0xfc, 0x78, x, y, 0x04, 0x00}};
  }

  constexpr ble_uuid128_t BaseUuid() {
    return CharUuid(0x00, 0x00);
  }

  constexpr ble_uuid128_t sleepServiceUuid {BaseUuid()};
  constexpr ble_uuid128_t sleepStateCharUuid {CharUuid(0x01, 0x00)};
  constexpr ble_uuid128_t sleepDurationCharUuid {CharUuid(0x02, 0x00)};
  constexpr ble_uuid128_t sleepQualityCharUuid {CharUuid(0x03, 0x00)};

  int SleepServiceCallback(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt* ctxt, void* arg) {
    auto* sleepService = static_cast<SleepService*>(arg);
    return sleepService->OnSleepRequested(conn_handle, attr_handle, ctxt);
  }
}

SleepService::SleepService(Pinetime::System::SystemTask& system, Controllers::SleepController& sleepController)
  : system {system},
    sleepController {sleepController},
    characteristicDefinition {{.uuid = &sleepStateCharUuid.u,
                               .access_cb = SleepServiceCallback,
                               .arg = this,
                               .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                               .val_handle = &sleepStateHandle},
                              {.uuid = &sleepDurationCharUuid.u,
                               .access_cb = SleepServiceCallback,
                               .arg = this,
                               .flags = BLE_GATT_CHR_F_READ,
                               .val_handle = &sleepDurationHandle},
                              {.uuid = &sleepQualityCharUuid.u,
                               .access_cb = SleepServiceCallback,
                               .arg = this,
                               .flags = BLE_GATT_CHR_F_READ,
                               .val_handle = &sleepQualityHandle},
                              {0}},
    serviceDefinition {
      {.type = BLE_GATT_SVC_TYPE_PRIMARY, .uuid = &sleepServiceUuid.u, .characteristics = characteristicDefinition},
      {0},
    } {
}

void SleepService::Init() {
  int res = 0;
  res = ble_gatts_count_cfg(serviceDefinition);
  ASSERT(res == 0);

  res = ble_gatts_add_svcs(serviceDefinition);
  ASSERT(res == 0);
}

int SleepService::OnSleepRequested(uint16_t connectionHandle, uint16_t attributeHandle, ble_gatt_access_ctxt* context) {
  (void)connectionHandle;
  if (attributeHandle == sleepStateHandle) {
    NRF_LOG_INFO("Sleep-state : handle = %d", sleepStateHandle);
    uint8_t state = static_cast<uint8_t>(sleepController.CurrentState());
    int res = os_mbuf_append(context->om, &state, 1);
    return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  } else if (attributeHandle == sleepDurationHandle) {
    NRF_LOG_INFO("Sleep-duration : handle = %d", sleepDurationHandle);
    uint32_t duration = sleepController.LastSleepDurationMin();
    int res = os_mbuf_append(context->om, &duration, 4);
    return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  } else if (attributeHandle == sleepQualityHandle) {
    NRF_LOG_INFO("Sleep-quality : handle = %d", sleepQualityHandle);
    uint8_t quality = sleepController.LastSleepQuality();
    int res = os_mbuf_append(context->om, &quality, 1);
    return (res == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }
  return 0;
}

void SleepService::OnNewSleepState() {
  if (!sleepStateNoficationEnabled)
    return;

  uint8_t state = static_cast<uint8_t>(sleepController.CurrentState());
  auto* om = ble_hs_mbuf_from_flat(&state, 1);

  uint16_t connectionHandle = system.nimble().connHandle();

  if (connectionHandle == 0 || connectionHandle == BLE_HS_CONN_HANDLE_NONE) {
    return;
  }

  ble_gattc_notify_custom(connectionHandle, sleepStateHandle, om);
}

void SleepService::SubscribeNotification(uint16_t connectionHandle, uint16_t attributeHandle) {
  (void)connectionHandle;
  if (attributeHandle == sleepStateHandle)
    sleepStateNoficationEnabled = true;
}

void SleepService::UnsubscribeNotification(uint16_t connectionHandle, uint16_t attributeHandle) {
  (void)connectionHandle;
  if (attributeHandle == sleepStateHandle)
    sleepStateNoficationEnabled = false;
}