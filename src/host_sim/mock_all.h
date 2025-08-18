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
  if (first) { first = 0; return 1; }  // trigger once
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
/* Sim state (degrees, but controller expects hundredths from IMU) */
static float sim_yaw_deg   = 0.0f;
static float sim_pitch_deg = 0.0f;
static float sim_roll_deg  = 60.0f;  // start on right side so recovery triggers

/* Latest commands from set_motors */
static int8_t g_last_yaw_cmd   = 0;
static int8_t g_last_pitch_cmd = 0;

/* Convert to device format (hundredths of degrees) */
struct imu_data_t { int16_t yaw; int16_t pitch; int16_t roll; };

/* Simple physics: integrate commands into angles */
inline imu_data_t get_imu_data() {
  // scale: ~0.2° per tick per PWM unit (tweak as you like)
  sim_yaw_deg   += (float)g_last_yaw_cmd   * 0.2f;
  sim_pitch_deg += (float)g_last_pitch_cmd * 0.2f;

  // normalize to [-180, 180]
  auto norm = [](float a){ while(a<=-180) a+=360; while(a>180) a-=360; return a; };
  sim_yaw_deg   = norm(sim_yaw_deg);
  sim_pitch_deg = norm(sim_pitch_deg);

  // roll stays at +60° during the flip planning; the controller doesn’t need it afterwards
  return imu_data_t{
    (int16_t)std::lround(sim_yaw_deg   * 100.0f),
    (int16_t)std::lround(sim_pitch_deg * 100.0f),
    (int16_t)std::lround(sim_roll_deg  * 100.0f)
  };
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
  g_last_yaw_cmd   = a->yaw;
  g_last_pitch_cmd = a->pitch;
  std::printf("[SIM] set_motors: yaw=%d pitch=%d drive=%d mode=%d\n",
              (int)a->yaw, (int)a->pitch, (int)a->drive, (int)a->flip_mode);
}



/* ===================== RSBL8512 stub ================== */
/* Needed because FlipController has RSBL8512& members and ctor args */
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
