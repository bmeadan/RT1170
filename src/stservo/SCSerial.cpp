/*
 * SCSerial.h
 * hardware interface layer for waveshare serial bus servo
 * date: 2023.6.28
 */

#include "SCSerial.h"
#include <stdint.h>
#include <stdio.h>

extern "C" {
#include "platform/platform.h"
#include "serial/serial_lpuart_485.h"
}

SCSerial::SCSerial() {
    IOTimeOut = 100;
}

SCSerial::SCSerial(u8 End)
    : SCS(End) {
    IOTimeOut = 100;
}

SCSerial::SCSerial(u8 End, u8 Level)
    : SCS(End, Level) {
    IOTimeOut = 100;
}

int SCSerial::readSCS(unsigned char *nDat, int nLen) {
    int Size = 0;
    uint8_t ComData;
    unsigned long t_begin = get_time();
    unsigned long t_user;
    return serial_lpuart_recv_bytes_485((uint8_t *)nDat, nLen);
}

int SCSerial::writeSCS(unsigned char *nDat, int nLen) {
    if (nDat == NULL) {
        return 0;
    }
    return serial_lpuart_send_bytes_485(nDat, nLen);
}

int SCSerial::writeSCS(unsigned char bDat) {
    return serial_lpuart_send_bytes_485(&bDat, 1);
}

void SCSerial::rFlushSCS() {
    serial_lpuart_read_flush_485();
}

void SCSerial::wFlushSCS() {
    serial_lpuart_write_flush_485();
}