#include "components/ble/FindPhoneService.h"
#include "components/ble/NimbleController.h"
#include <nrf_log.h>

using namespace Pinetime::Controllers;

namespace {
  // 0006yyxx-78fc-48fe-8e23-433b3a1942d0
  constexpr ble_uuid128_t CharUuid(uint8_t x, uint8_t y) {
    return ble_uuid128_t {.u = {.type = BLE_UUID_TYPE_128},
                          .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e, 0xfe, 0x48, 0xfc, 0x78, x, y, 0x06, 0x00}};
  }

  // 00060000-78fc-48fe-8e23-433b3a1942d0
  constexpr ble_uuid128_t BaseUuid() {
    return CharUuid(0x00, 0x00);
  }

  constexpr ble_uuid128_t findPhoneServiceUuid {BaseUuid()};
  constexpr ble_uuid128_t ringCharUuid {CharUuid(0x01, 0x00)};

  int FindPhoneServiceCallback(uint16_t /*conn_handle*/, uint16_t attr_handle, struct ble_gatt_access_ctxt* ctxt, void* arg) {
    auto* findPhoneService = static_cast<FindPhoneService*>(arg);
    return findPhoneService->OnWrite(attr_handle, ctxt);
  }
}

FindPhoneService::FindPhoneService(NimbleController& nimble)
  : nimble {nimble},
    characteristicDefinition {{.uuid = &ringCharUuid.u,
                               .access_cb = FindPhoneServiceCallback,
                               .arg = this,
                               .flags = BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_WRITE,
                               .val_handle = &ringHandle},
                              {0}},
    serviceDefinition {
      {.type = BLE_GATT_SVC_TYPE_PRIMARY, .uuid = &findPhoneServiceUuid.u, .characteristics = characteristicDefinition},
      {0},
    } {
}

void FindPhoneService::Init() {
  int res = 0;
  res = ble_gatts_count_cfg(serviceDefinition);
  ASSERT(res == 0);

  res = ble_gatts_add_svcs(serviceDefinition);
  ASSERT(res == 0);
}

void FindPhoneService::Ring() {
  if (!ringNotificationEnabled) {
    NRF_LOG_INFO("FindPhone : ring requested but no subscriber");
    return;
  }

  uint16_t connectionHandle = nimble.connHandle();
  if (connectionHandle == 0 || connectionHandle == BLE_HS_CONN_HANDLE_NONE) {
    NRF_LOG_INFO("FindPhone : ring requested but not connected");
    return;
  }

  uint8_t buffer = 1;
  auto* om = ble_hs_mbuf_from_flat(&buffer, 1);
  ble_gattc_notify_custom(connectionHandle, ringHandle, om);
  NRF_LOG_INFO("FindPhone : ring notification sent");
}

int FindPhoneService::OnWrite(uint16_t attributeHandle, ble_gatt_access_ctxt* context) {
  if (attributeHandle != ringHandle) {
    return 0;
  }

  if (context->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
    return 0;
  }

  if (context->om->om_len < 1) {
    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
  }

  uint8_t value = 0;
  os_mbuf_copydata(context->om, 0, 1, &value);
  NRF_LOG_INFO("FindPhone : phone wrote %d", value);
  // 0x00 = phone acknowledged/stopped ringing; 0x01 = phone started ringing
  // (reserved for future use — UI feedback on the watch)
  return 0;
}

void FindPhoneService::SubscribeNotification(uint16_t attributeHandle) {
  if (attributeHandle == ringHandle) {
    ringNotificationEnabled = true;
  }
}

void FindPhoneService::UnsubscribeNotification(uint16_t attributeHandle) {
  if (attributeHandle == ringHandle) {
    ringNotificationEnabled = false;
  }
}
