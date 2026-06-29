#include "components/heartrate/HeartRateController.h"
#include <heartratetask/HeartRateTask.h>
#include <systemtask/SystemTask.h>

using namespace Pinetime::Controllers;

void HeartRateController::Update(HeartRateController::States newState, uint8_t heartRate) {
  this->state = newState;
  if (this->heartRate != heartRate) {
    this->heartRate = heartRate;
    service->OnNewHeartRateValue(heartRate);
  }
}

void HeartRateController::Enable() {
#ifdef DRIVER_ACC_SC7A20
  if (task != nullptr) {
    state = States::NotEnoughData;
    task->PushMessage(Pinetime::Applications::HeartRateTask::Messages::StartMeasurement);
  }
#else
  if (task != nullptr) {
    state = States::NotEnoughData;
    task->PushMessage(Pinetime::Applications::HeartRateTask::Messages::Enable);
  }
#endif
}

void HeartRateController::Disable() {
#ifdef DRIVER_ACC_SC7A20
  if (task != nullptr) {
    state = States::Stopped;
    task->PushMessage(Pinetime::Applications::HeartRateTask::Messages::StopMeasurement);
  }
#else
  if (task != nullptr) {
    state = States::Stopped;
    task->PushMessage(Pinetime::Applications::HeartRateTask::Messages::Disable);
  }
#endif
}

void HeartRateController::SetHeartRateTask(Pinetime::Applications::HeartRateTask* task) {
  this->task = task;
}

void HeartRateController::SetService(Pinetime::Controllers::HeartRateService* service) {
  this->service = service;
}