#ifndef __FLEXSPI_NOR_FLASH_OPS_H__
#define __FLEXSPI_NOR_FLASH_OPS_H__

#include "fsl_flexspi.h"
#include <stdint.h>

// When running within the bootloader context
// We need to enable the XIP boot header
#define XIP_BOOT_HEADER_ENABLE_INSIDE defined(DEBUG) ? 0 : 1

#define FLASH_SIZE                    0x4000 /* 16Mb/KByte */
#define FLASH_FLEXSPI                 FLEXSPI1
#define FLASH_FLEXSPI_AMBA_BASE       FlexSPI1_AMBA_BASE
#define SECTOR_SIZE                   0x1000   /* 4K */
#define ERASE_SIZE                    0x200000 // 2MB
#define SECTOR_COUNT                  (ERASE_SIZE / SECTOR_SIZE)
#define FLASH_PAGE_SIZE               256
#define FLASH_FLEXSPI_CLOCK           kCLOCK_Flexspi1
#define FLASH_PORT                    kFLEXSPI_PortA1
#define FLASH_FLEXSPI_RX_SAMPLE_CLOCK kFLEXSPI_ReadSampleClkLoopbackFromDqsPad

#define NOR_CMD_LUT_SEQ_IDX_READ_NORMAL        7
#define NOR_CMD_LUT_SEQ_IDX_READ_FAST          13
#define NOR_CMD_LUT_SEQ_IDX_READ_FAST_QUAD     0
#define NOR_CMD_LUT_SEQ_IDX_READSTATUS         1
#define NOR_CMD_LUT_SEQ_IDX_WRITEENABLE        2
#define NOR_CMD_LUT_SEQ_IDX_ERASESECTOR        3
#define NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_SINGLE 6
#define NOR_CMD_LUT_SEQ_IDX_PAGEPROGRAM_QUAD   4
#define NOR_CMD_LUT_SEQ_IDX_READID             8
#define NOR_CMD_LUT_SEQ_IDX_WRITESTATUSREG     9
#define NOR_CMD_LUT_SEQ_IDX_ENTERQPI           10
#define NOR_CMD_LUT_SEQ_IDX_EXITQPI            11
#define NOR_CMD_LUT_SEQ_IDX_READSTATUSREG      12
#define NOR_CMD_LUT_SEQ_IDX_ERASECHIP          5

#define CUSTOM_LUT_LENGTH            64U
#define FLASH_QUAD_ENABLE            0x40U
#define FLASH_BUSY_STATUS_POL        1U
#define FLASH_BUSY_STATUS_OFFSET     0U
#define FLASH_DUMMY_CYCLES           9U
#define BURST_LEGNTH_REG_SHIFT       0U
#define WRAP_ENABLE_REG_SHIFT        2U
#define DUMMY_CYCLES_REG_SHIFT       3U
#define RESET_PIN_SELECTED_REG_SHIFT 7U
#define CACHE_MAINTAIN               1

#if (defined CACHE_MAINTAIN) && (CACHE_MAINTAIN == 1)
typedef struct _flexspi_cache_status {
    volatile bool DCacheEnableFlag;
    volatile bool ICacheEnableFlag;
} flexspi_cache_status_t;
#endif

static inline void flexspi_clock_init(void) {
    /*Clock setting for flexspi1*/
    CLOCK_SetRootClockDiv(kCLOCK_Root_Flexspi1, 2);
    CLOCK_SetRootClockMux(kCLOCK_Root_Flexspi1, 0);
}

status_t flexspi_nor_flash_erase_sector(FLEXSPI_Type *base, uint32_t address);
status_t flexspi_nor_flash_program(FLEXSPI_Type *base, uint32_t dstAddr, const uint32_t *src, uint32_t length);
status_t flexspi_nor_flash_read(FLEXSPI_Type *base, uint32_t dstAddr, const uint32_t *src, uint32_t length);
status_t flexspi_nor_flash_page_program(FLEXSPI_Type *base, uint32_t dstAddr, const uint32_t *src);
status_t flexspi_nor_get_vendor_id(FLEXSPI_Type *base, uint8_t *vendorId);
status_t flexspi_nor_enable_quad_mode(FLEXSPI_Type *base);
status_t flexspi_nor_erase_chip(FLEXSPI_Type *base);
void flexspi_nor_flash_init(FLEXSPI_Type *base);

#endif // __FLEXSPI_NOR_FLASH_OPS_H__