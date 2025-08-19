#pragma once
#include <cstdint>
#include <cstdio>
#include <thread>
#include <chrono>
#include <cmath>
#include <atomic>

/* ====================== Platform ====================== */
inline void platform_init() {}
inline uint32_t platform_millis() { static uint32_t t=0; return ++t; }
inline void platform_log(const char* s) { std::printf("[LOG] %s\n", s); }

/* ======================= FreeRTOS ===================== */
using TickType_t   = uint32_t;
using BaseType_t   = int;
using TaskHandle_t = void*;

#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) (ms)
#endif
#ifndef pdTRUE
#define pdTRUE 1
#endif
#ifndef pdFALSE
#define pdFALSE 0
#endif
#ifndef tskIDLE_PRIORITY
#define tskIDLE_PRIORITY 0
#endif
#ifndef configMAX_PRIORITIES
#define configMAX_PRIORITIES 5
#endif

inline void vTaskDelay(TickType_t ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
inline void vTaskDelete(void*) {}
inline BaseType_t xTaskCreate(void (*fn)(void*), const char*, uint16_t,
                              void* param, uint32_t, TaskHandle_t*) {
  std::thread([=]{ fn(param); }).detach();
  return pdTRUE;
}
inline uint32_t ulTaskNotifyTake(BaseType_t, TickType_t) {
  static int first = 1;
  if (first) { first = 0; return 1; }
  return 0;
}
inline void     xTaskNotifyGive(TaskHandle_t) {}
inline void     vTaskNotifyGiveFromISR(TaskHandle_t, BaseType_t*) {}
inline void     portYIELD_FROM_ISR(BaseType_t) {}


/* ======================= Audio ======================== */
inline void audio_init() {}
inline void audio_play(int) {}

/* ======================= Lights ======================= */
inline void lights_set(int, int) {}
inline void lights_blink(int, int) {}

/* ======================== IMU ========================= */
struct imu_data_t {
  int16_t yaw;
  int16_t pitch;
  // roll removed
};

struct SimIMU {
  float pitch_deg = 0.0f;
  float yaw_deg   = 0.0f;
};

extern SimIMU g_imu;

inline imu_data_t get_imu_data() {
  imu_data_t out;
  out.yaw   = static_cast<int16_t>(std::lround(g_imu.yaw_deg * 100.0f));
  out.pitch = static_cast<int16_t>(std::lround(g_imu.pitch_deg * 100.0f));
  return out;
}

/* ==================== Motors / Actions ================ */
struct motors_action_t {
  int8_t drive = 0;
  int8_t rotate = 0;
  int8_t yaw = 0;
  int8_t pitch = 0;
  int8_t auto_neutral_joints = 0;
  int8_t drive_pulse = 0;
  int8_t flip_mode = 0;
};

inline void motors_init() {}

inline void set_motors(const motors_action_t* a) {
  static int8_t py=127, pp=127, pd=127, pm=127;
  if (a->yaw==py && a->pitch==pp && a->drive==pd && a->flip_mode==pm) return;
  py=a->yaw; pp=a->pitch; pd=a->drive; pm=a->flip_mode;

  std::printf("[SIM] set_motors: yaw=%d pitch=%d drive=%d mode=%d\n",
              (int)a->yaw, (int)a->pitch, (int)a->drive, (int)a->flip_mode);
}


/* ===================== RSBL8512 stub ================== */
struct RSBL8512 {
  explicit RSBL8512(uint8_t) {}
  void setTarget(int) {}
  int  position() const { return 0; }
};

/* =================== USB / Video ====================== */
typedef struct { int address; int fragment; } api_message_t;
typedef struct { bool ready; int size[4]; } frame_data_t;
extern frame_data_t frames[4];
inline void usb_host_init() {}

/* ======================== Flash ======================= */
inline void flash_init() {}
inline int  flash_read(int, void*, int) { return 0; }
inline int  flash_write(int, const void*, int) { return 0; }

/* ============== External Flash Map / Net ============== */
inline int  ext_map_addr(const char*) { return 0x10000; }
inline void network_init() {}
inline void network_poll() {}
