#ifndef _SERIAL_LPUART_H_
#define _SERIAL_LPUART_H_

#include "fsl_lpuart.h"

#ifdef __cplusplus
extern "C" {
#endif

void init_serial_lpuart_232(void);
int serial_lpuart_send_bytes_232(const uint8_t *data, size_t length);
int serial_lpuart_recv_bytes_232(uint8_t *data, uint16_t len);
void serial_lpuart_read_flush_232(void);
void serial_lpuart_write_flush_232(void);

#ifdef __cplusplus
}
#endif

#endif // _SERIAL_LPUART_H_