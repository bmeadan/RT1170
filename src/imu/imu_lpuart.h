#ifndef _SERIAL_LPUART_IMU_H_
#define _SERIAL_LPUART_IMU_H_

#define bool _Bool
#include "fsl_lpuart.h"

void init_serial_lpuart(void);
void init_imu_lpuart(void);
bool UART_ReadFromRingBuffer(uint8_t *data, uint16_t len);

#endif // _SERIAL_LPUART_IMU_H_