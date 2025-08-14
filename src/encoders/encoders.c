/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "encoders.h"
#include "board.h"
#include "clock_config.h"
#include "logger/logger.h"
#include "fsl_enc.h"
#include "fsl_xbara.h"
#include "pin_mux.h"
#include "platform/platform.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define DEMO_ENC_BASEADDR ENC1

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

static unsigned long prev_encoder_time = 0;
static float measured_velocity         = 0;

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief init function
 */
void init_encoder(void) {
    enc_config_t mEncConfigStruct;

    XBARA_Init(XBARA1);
    XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputIomuxXbarInout38, kXBARA1_OutputDec1Phasea);
    XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputIomuxXbarInout39, kXBARA1_OutputDec1Phaseb);
    XBARA_SetSignalsConnection(XBARA1, kXBARA1_InputIomuxXbarInout40, kXBARA1_OutputDec1Index);

    LOGF("ENC Basic Example.");

    /* Initialize the ENC module. */
    ENC_GetDefaultConfig(&mEncConfigStruct);
    ENC_Init(DEMO_ENC_BASEADDR, &mEncConfigStruct);
    ENC_DoSoftwareLoadInitialPositionValue(DEMO_ENC_BASEADDR); /* Update the position counter with initial value. */
}

void print_relative_encoder() {

    uint32_t mCurPosValue;

    /* This read operation would capture all the position counter to responding hold registers. */
    mCurPosValue = ENC_GetPositionValue(DEMO_ENC_BASEADDR);

    /* Read the position values. */
    LOGF("Current position value: %ld", mCurPosValue);
    LOGF("Position differential value: %d", (int16_t)ENC_GetHoldPositionDifferenceValue(DEMO_ENC_BASEADDR));
    LOGF("Position revolution value: %d", ENC_GetHoldRevolutionValue(DEMO_ENC_BASEADDR));
}

#define COUNTS_TO_ROTATION 1.0

void update_drive_encoder() {

    uint32_t mCurPosValue;
    uint16_t mCurPosDiffValue;
    int16_t signed_position_diff;
    unsigned long current_time = get_time();
    int delta_t;
    float counts_per_msec;

    /* Capture position counter */
    mCurPosValue     = ENC_GetPositionValue(DEMO_ENC_BASEADDR);
    mCurPosDiffValue = ENC_GetPositionDifferenceValue(DEMO_ENC_BASEADDR);

    /* Cast the different to signed using 2's comp*/
    signed_position_diff = (int16_t)mCurPosDiffValue;

    /* Calculate the velocity */
    delta_t = (int)(current_time - prev_encoder_time);
    if (delta_t == 0)
        LOGF("time diff is zero");
    else {
        counts_per_msec   = (signed_position_diff / delta_t) * 100.0;
        measured_velocity = counts_per_msec * COUNTS_TO_ROTATION;

        // Update last encoder time
        prev_encoder_time = current_time;
    }

    // Debug
    // LOGF("Current position value: %ld", mCurPosValue);
    LOGF("Current Position diff: %d", signed_position_diff);
    LOGF("Current velocity (RPM) %d delta_t = %d", (int)measured_velocity, delta_t);
}
