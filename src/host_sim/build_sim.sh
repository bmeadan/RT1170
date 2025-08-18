#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

TMPDIR="$(mktemp -d)"
cleanup(){ rm -rf "$TMPDIR"; }
trap cleanup EXIT

# -------------------- FreeRTOS core stubs --------------------
cat > "$TMPDIR/FreeRTOS.h" <<'EOF'
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t TickType_t;
typedef int32_t  BaseType_t;
typedef uint32_t UBaseType_t;
typedef void*    TaskHandle_t;

#ifndef configTICK_RATE_HZ
#define configTICK_RATE_HZ 1000UL
#endif
#ifndef portTICK_PERIOD_MS
#define portTICK_PERIOD_MS (1000UL / configTICK_RATE_HZ)
#endif
#ifndef pdTRUE
#define pdTRUE 1
#endif
#ifndef pdFALSE
#define pdFALSE 0
#endif
#ifndef portMAX_DELAY
#define portMAX_DELAY 0xFFFFFFFFu
#endif
#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) ((TickType_t)((ms) / portTICK_PERIOD_MS))
#endif
#ifndef configMAX_PRIORITIES
#define configMAX_PRIORITIES 5
#endif

#ifdef __cplusplus
}
#endif
EOF

cat > "$TMPDIR/task.h" <<'EOF'
#pragma once
#include "FreeRTOS.h"
#ifdef __cplusplus
extern "C" {
#endif

// Host-sim task/notify implemented with a single global mailbox.
// Good enough for one FlipController task.
typedef struct {
  volatile uint32_t notif_count;
} __SimTask;
static __SimTask __sim_task = {0};

static inline void vTaskDelay(TickType_t){ /* no-op; host main drives time */ }
static inline TickType_t xTaskGetTickCount(void){ static TickType_t t=0; return ++t; }

typedef void (*TaskFunction_t)(void*);

static inline BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, const char*, uint16_t,
                                     void* pvParameters, UBaseType_t, TaskHandle_t* pxCreatedTask) {
  // Run the task function once in a detached host thread.
  // The task is expected to loop internally.
  (void)pxCreatedTask;
  (void)pvParameters;
  (void)pxTaskCode;
  // We won't actually start a real thread to keep deps minimal.
  // The Flip task should be called via fc.loop() internally when notified.
  return pdTRUE;
}

static inline void vTaskDelete(void*) { }

static inline uint32_t ulTaskNotifyTake(BaseType_t clearOnExit, TickType_t /*ticksToWait*/) {
  uint32_t n = __sim_task.notif_count;
  if (clearOnExit && n) __sim_task.notif_count = 0;
  return n;
}

static inline void xTaskNotifyGive(TaskHandle_t){ __sim_task.notif_count += 1; }

#ifdef __cplusplus
}
#endif
EOF

cat > "$TMPDIR/semphr.h" <<'EOF'
#pragma once
#include "FreeRTOS.h"
typedef void* SemaphoreHandle_t;
static inline SemaphoreHandle_t xSemaphoreCreateMutex(void){ return (SemaphoreHandle_t)1; }
static inline void vSemaphoreDelete(SemaphoreHandle_t){ }
static inline BaseType_t xSemaphoreTake(SemaphoreHandle_t, TickType_t){ return pdTRUE; }
static inline BaseType_t xSemaphoreGive(SemaphoreHandle_t){ return pdTRUE; }
EOF

cat > "$TMPDIR/portmacro.h" <<'EOF'
#pragma once
#define portYIELD_FROM_ISR(x) (void)(x)
EOF

# -------------------- NXP SDK stubs --------------------
cat > "$TMPDIR/fsl_common.h" <<'EOF'
#pragma once
#include <stdint.h>
typedef int32_t status_t;
#ifndef __STATIC_INLINE
#define __STATIC_INLINE static inline
#endif
#ifndef SDK_ALIGN
#define SDK_ALIGN(x, y) x
#endif
#ifndef SDK_SIZEALIGN
#define SDK_SIZEALIGN(x, y) x
#endif
#ifndef __NOP
#define __NOP() ((void)0)
#endif
EOF

# Headers often pulled transitively; make them harmless.
for H in fsl_gpio.h fsl_clock.h fsl_iomuxc.h fsl_iopctl.h fsl_debug_console.h fsl_trng.h; do
  cat > "$TMPDIR/$H" <<'EOF'
#pragma once
/* host-sim stub */
EOF
done

echo "[build_sim] Compiling…"
g++ -std=c++17 -O0 -g -pthread -DHOST_SIM \
  -I"$TMPDIR" -I. -I.. -I../.. \
  sim.cpp ../controllers/FlipController.cpp \
  -o sim

echo "[build_sim] Built ./sim"
echo "[build_sim] Run it now with: ./sim"
