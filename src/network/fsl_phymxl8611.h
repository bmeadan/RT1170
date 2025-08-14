/*
 * Copyright 2020-2023 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_PHYMXL8611F_H_
#define _FSL_PHYMXL8611F_H_

#include "fsl_phy.h"

typedef struct _phy_mxl8611_resource {
    mdioWrite write;
    mdioRead read;
} phy_mxl8611_resource_t;

extern const phy_operations_t phymxl8611_ops;

/*******************************************************************************
 * API
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @name PHY Driver
 * @{
 */

/*!
 * @brief Initializes PHY.
 * This function initializes PHY.
 *
 * @param handle  PHY device handle.
 * @param config  PHY configuration.
 * @retval kStatus_Success  PHY initialization succeeds
 * @retval kStatus_Fail  PHY initialization fails
 * @retval kStatus_Timeout  PHY MDIO visit time out
 */
status_t PHY_MXL8611_Init(phy_handle_t *handle, const phy_config_t *config);

/*!
 * @brief PHY Write function.
 * This function writes data over the MDIO to the specified PHY register.
 *
 * @param handle  PHY device handle.
 * @param phyReg  The PHY register.
 * @param data    The data written to the PHY register.
 * @retval kStatus_Success     PHY write success
 * @retval kStatus_Timeout  PHY MDIO visit time out
 */
status_t PHY_MXL8611_Write(phy_handle_t *handle, uint8_t phyReg, uint16_t data);

/*!
 * @brief PHY Read function.
 * This interface reads data over the MDIO from the specified PHY register.
 *
 * @param handle  PHY device handle.
 * @param phyReg  The PHY register.
 * @param pData   The address to store the data read from the PHY register.
 * @retval kStatus_Success  PHY read success
 * @retval kStatus_Timeout  PHY MDIO visit time out
 */
status_t PHY_MXL8611_Read(phy_handle_t *handle, uint8_t phyReg, uint16_t *pData);

/*!
 * @brief Gets the PHY auto-negotiation status.
 *
 * @param handle  PHY device handle.
 * @param status  The auto-negotiation status of the PHY.
 *         - true the auto-negotiation is over.
 *         - false the auto-negotiation is on-going or not started.
 * @retval kStatus_Success  PHY gets status success
 * @retval kStatus_Timeout  PHY MDIO visit time out
 */
status_t PHY_MXL8611_GetAutoNegotiationStatus(phy_handle_t *handle, bool *status);

/*!
 * @brief Gets the PHY link status.
 *
 * @param handle   PHY device handle.
 * @param status   The link up or down status of the PHY.
 *         - true the link is up.
 *         - false the link is down.
 * @retval kStatus_Success   PHY gets link status success
 * @retval kStatus_Timeout  PHY MDIO visit time out
 */
status_t PHY_MXL8611_GetLinkStatus(phy_handle_t *handle, bool *status);

/*!
 * @brief Gets the PHY link speed and duplex.
 *
 * @brief This function gets the speed and duplex mode of PHY. User can give one of speed
 * and duplex address paramter and set the other as NULL if only wants to get one of them.
 *
 * @param handle   PHY device handle.
 * @param speed    The address of PHY link speed.
 * @param duplex   The link duplex of PHY.
 * @retval kStatus_Success   PHY gets link speed and duplex success
 * @retval kStatus_Timeout  PHY MDIO visit time out
 */
status_t PHY_MXL8611_GetLinkSpeedDuplex(phy_handle_t *handle, phy_speed_t *speed, phy_duplex_t *duplex);

/*!
 * @brief Sets the PHY link speed and duplex.
 *
 * @param handle   PHY device handle.
 * @param speed    Specified PHY link speed.
 * @param duplex   Specified PHY link duplex.
 * @retval kStatus_Success   PHY gets status success
 * @retval kStatus_Timeout  PHY MDIO visit time out
 */
status_t PHY_MXL8611_SetLinkSpeedDuplex(phy_handle_t *handle, phy_speed_t speed, phy_duplex_t duplex);

/*!
 * @brief Enables/Disables PHY loopback.
 *
 * @param handle   PHY device handle.
 * @param mode     The loopback mode to be enabled, please see "phy_loop_t".
 * All loopback modes should not be set together, when one loopback mode is set
 * another should be disabled.
 * @param speed    PHY speed for loopback mode.
 * @param enable   True to enable, false to disable.
 * @retval kStatus_Success  PHY loopback success
 * @retval kStatus_Timeout  PHY MDIO visit time out
 */
status_t PHY_MXL8611_EnableLoopback(phy_handle_t *handle, phy_loop_t mode, phy_speed_t speed, bool enable);

/*!
 * @brief Enables/Disables PHY link management interrupt.
 *
 * This function controls link status change interrupt.
 *
 * @param handle  PHY device handle.
 * @param type    PHY interrupt type.
 * @retval kStatus_Success  PHY enables/disables interrupt success
 * @retval kStatus_Timeout  PHY MDIO visit time out
 */
status_t PHY_MXL8611_EnableLinkInterrupt(phy_handle_t *handle, phy_interrupt_type_t type);

/*!
 * @brief Clears PHY interrupt status.
 *
 * @param handle  PHY device handle.
 * @retval kStatus_Success  PHY read and clear interrupt success
 * @retval kStatus_Timeout  PHY MDIO visit time out
 */
status_t PHY_MXL8611_ClearInterrupt(phy_handle_t *handle);

/* @} */

#ifdef __cplusplus
}
#endif

/*! @}*/

#endif /* _FSL_PHY_H_ */
