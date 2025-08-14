#include <thread>
#include <atomic>
#include <chrono>
#include <cstdio>

// Put mocks first
#include "mocks/FreeRTOS.h"
#include "mocks/task.h"
#include "mocks/motors/motors.h"
#include "mocks/motors/RSBL8512.h"
#include "mocks/imu/imu.h"

// Include your real controller (adjust if it's under src/controllers)
#include "../controllers/FlipController.h"

// ------- FreeRTOS shim -------
using namespace std::chrono;
static std::atomic<uint32_t> gNotify{0};
static std::thread gTaskThread;

extern "C" void vTaskDelay(TickType_t t){ std::this_thread::sleep_for(milliseconds(t)); }
extern "C" BaseType_t xTaskCreate(void (*task)(void*), const char*, uint16_t, void* arg, uint32_t, TaskHandle_t* out){
  gTaskThread = std::thread([=]{ task(arg); });
  gTaskThread.detach();
  if(out) *out = (TaskHandle_t)0x1;
  return pdTRUE;
}
extern "C" uint32_t ulTaskNotifyTake(BaseType_t clr, TickType_t){ uint32_t n=gNotify.load(); if(n&&clr) gNotify.store(0); return n; }
extern "C" void vTaskDelete(TaskHandle_t){ }
extern "C" void vTaskNotifyGiveFromISR(TaskHandle_t, BaseType_t*){ gNotify.store(1); }
extern "C" void xTaskNotifyGive(TaskHandle_t){ gNotify.store(1); }
extern "C" void portYIELD_FROM_ISR(BaseType_t){ }

// IMU stub (real values come from sim hooks in FlipController.cpp)
extern "C" imu_data_t get_imu_data(void){ return {0,0,0}; }

// Hooks implemented inside FlipController.cpp (we added earlier)
extern void flip_bind(RSBL8512& yaw, RSBL8512& pitch);
extern "C" void flip_sim_left(void);
extern "C" void flip_sim_right(void);
extern "C" void flip_sim_back(void);

int main(){
  RSBL8512 yawEnc, pitchEnc;
  flip_bind(yawEnc, pitchEnc);             // starts the controller "task"

  std::this_thread::sleep_for(milliseconds(500));

  std::puts("\n=== SIM: LEFT SIDE ===");  flip_sim_left();
  std::this_thread::sleep_for(seconds(5));

  std::puts("\n=== SIM: RIGHT SIDE ==="); flip_sim_right();
  std::this_thread::sleep_for(seconds(5));

  std::puts("\n=== SIM: BACK ===");       flip_sim_back();
  std::this_thread::sleep_for(seconds(5));

  std::puts("\n=== SIM: DONE ===");
  return 0;
}
