#ifndef _PROTOCOL_ERROR_H
#define _PROTOCOL_ERROR_H

typedef enum protocol_error {
    PROTOCOL_ERROR_NONE            = 0,
    PROTOCOL_ERROR_PREAMBLE        = 1,
    PROTOCOL_ERROR_CHECKSUM        = 2,
    PROTOCOL_ERROR_LENGTH          = 3,
    PROTOCOL_ERROR_INVALID_PAYLOAD = 4,
    PROTOCOL_ERROR_UNKNOWN_TYPE    = 5,
    PROTOCOL_ERROR_UNKNOWN_OPCODE  = 6,
} protocol_error_t;

#endif // _PROTOCOL_ERROR_H