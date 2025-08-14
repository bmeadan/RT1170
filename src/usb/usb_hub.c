/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "usb_hub.h"
#include "board.h"
#include "clock_config.h"
#include "fsl_common.h"
#include "pin_mux.h"
#include "usb_phy.h"

usb_host_handle g_HostHandle;
extern usb_host_video_camera_instance_t g_Cameras[MAX_USB_CAMERAS];


void USB_OTG1_IRQHandler(void) {
    USB_HostEhciIsrFunction(g_HostHandle);
}


void USB_OTG2_IRQHandler(void) {
    USB_HostEhciIsrFunction(g_HostHandle);
}

void USB_HostClockInit(void) {
    uint32_t usbClockFreq;
    usb_phy_config_struct_t phyConfig = {
        BOARD_USB_PHY_D_CAL,
        BOARD_USB_PHY_TXCAL45DP,
        BOARD_USB_PHY_TXCAL45DM,
    };
    usbClockFreq = 24000000;
    if (CONTROLLER_ID == kUSB_ControllerEhci0) {
        CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usbphy480M, usbClockFreq);
        CLOCK_EnableUsbhs0Clock(kCLOCK_Usb480M, usbClockFreq);
    } else {
        CLOCK_EnableUsbhs1PhyPllClock(kCLOCK_Usbphy480M, usbClockFreq);
        CLOCK_EnableUsbhs1Clock(kCLOCK_Usb480M, usbClockFreq);
    }
    USB_EhciPhyInit(CONTROLLER_ID, BOARD_XTAL0_CLK_HZ, &phyConfig);
}

void USB_HostIsrEnable(void) {
    uint8_t irqNumber;

    uint8_t usbHOSTEhciIrq[] = USBHS_IRQS;
    irqNumber                = usbHOSTEhciIrq[CONTROLLER_ID - kUSB_ControllerEhci0];
/* USB_HOST_CONFIG_EHCI */

/* Install isr, set priority, and enable IRQ. */
#if defined(__GIC_PRIO_BITS)
    GIC_SetPriority((IRQn_Type)irqNumber, USB_HOST_INTERRUPT_PRIORITY);
#else
    NVIC_SetPriority((IRQn_Type)irqNumber, USB_HOST_INTERRUPT_PRIORITY);
#endif
    EnableIRQ((IRQn_Type)irqNumber);
}


void USB_HostTaskFn(void *param) {
    USB_HostEhciTaskFunction(param);
}


static void debug_device_handle(usb_device_handle deviceHandle) {
    uint32_t host_device_address = 0;
    // uint32_t host_device_hub_number = 0;
    uint32_t host_device_port_number = 0;
    // uint32_t host_device_speed = 0;
    // uint32_t host_device_hs_hub_number = 0;
    uint32_t host_device_hs_hub_port = 0;
    // uint32_t host_device_level = 0;
    // uint32_t host_host_handle = 0;
    // uint32_t host_device_control_pipe = 0;
    // uint32_t host_device_pid = 0;
    // uint32_t host_device_vid = 0;
    // uint32_t host_hub_think_time = 0;
    // uint32_t host_device_config_index = 0;
    // uint32_t host_configuration_des = 0;
    // uint32_t host_configuration_length = 0;
    (void)USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceAddress, &host_device_address);
    // (void)USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceHubNumber, &host_device_hub_number);
    (void)USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDevicePortNumber, &host_device_port_number);
    // (void)USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceSpeed, &host_device_speed);
    // (void)USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceHSHubNumber, &host_device_hs_hub_number);
    (void)USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceHSHubPort, &host_device_hs_hub_port);
    // (void)USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceLevel, &host_device_level);
    // (void)USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetHostHandle, &host_host_handle);
    // (void)USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceControlPipe, &host_device_control_pipe);
    // (void)USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDevicePID, &host_device_pid);
    // (void)USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceVID, &host_device_vid);
    // (void)USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetHubThinkTime, &host_hub_think_time);
    // (void)USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDeviceConfigIndex, &host_device_config_index);
    // (void)USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetConfigurationDes, &host_configuration_des);
    // (void)USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetConfigurationLength, &host_configuration_length);

    usb_echo("Address: %d Port: %d Hub Port: %d\r\n", host_device_address, host_device_port_number, host_device_hs_hub_port);
}

/*!
 * @brief host callback function.
 *
 * device attach/detach callback function.
 *
 * @param deviceHandle           device handle.
 * @param configurationHandle attached device's configuration descriptor information.
 * @param eventCode           callback event code, please reference to enumeration host_event_t.
 *
 * @retval kStatus_USB_Success              The host is initialized successfully.
 * @retval kStatus_USB_NotSupported         The application don't support the configuration.
 */

static usb_status_t USB_HostEvent(usb_device_handle deviceHandle,
                                  usb_host_configuration_handle configurationHandle,
                                  uint32_t eventCode) {
    usb_status_t status = kStatus_USB_Success;
    switch (eventCode & 0x0000FFFFU) {
        case kUSB_HostEventAttach:
            usb_echo("event attach.\r\n");
            // debug_device_handle(deviceHandle);
            status = USB_HostVideoEvent(deviceHandle, configurationHandle, eventCode);
            break;

        case kUSB_HostEventNotSupported:
            usb_echo("device not supported.\r\n");
            break;

        case kUSB_HostEventEnumerationDone:
            usb_echo("enumeration done.\r\n");
            status = USB_HostVideoEvent(deviceHandle, configurationHandle, eventCode);
            break;

        case kUSB_HostEventDetach:
            usb_echo("event detach.\r\n");
            status = USB_HostVideoEvent(deviceHandle, configurationHandle, eventCode);
            break;

        case kUSB_HostEventEnumerationFail:
            usb_echo("enumeration failed\r\n");
            break;

        default:
            break;
    }
    return status;
}


void USB_HostApplicationInit(void) {
    usb_status_t status = kStatus_USB_Success;
    USB_HostClockInit();

#if ((defined FSL_FEATURE_SOC_SYSMPU_COUNT) && (FSL_FEATURE_SOC_SYSMPU_COUNT))
    SYSMPU_Enable(SYSMPU, 0);
#endif /* FSL_FEATURE_SOC_SYSMPU_COUNT */

    status = USB_HostInit(CONTROLLER_ID, &g_HostHandle, USB_HostEvent);
    if (status != kStatus_USB_Success) {
        usb_echo("host init error\r\n");
        return;
    }
    USB_HostIsrEnable();

    usb_echo("host init done\r\n");
}

static void USB_HostTask(void *param) {
    while (1) {
        USB_HostTaskFn(param);
    }

    vTaskDelete(NULL);
}

extern volatile uint8_t camera_stream_id = 0;
static void USB_VideoStreamsTask(void *param) {
    usb_host_video_camera_instance_t *cameras = (usb_host_video_camera_instance_t *)param;
    usb_host_video_camera_instance_t *camera;
    
    while (1) {
        for (int i = 0; i < MAX_USB_CAMERAS; i++) {
            USB_HostStreamVideoTask(&cameras[i]);
        }
    }

    vTaskDelete(NULL);
}

void init_usb_hub(void) {
    USB_HostApplicationInit();
    USB_HostVideoInitialize();

    if (xTaskCreate(USB_HostTask, "usb host", 2500L / sizeof(portSTACK_TYPE), (void *)g_HostHandle, configMAX_PRIORITIES - 3, NULL) != pdPASS) {
        usb_echo("create host task error\r\n");
    }

    if (xTaskCreate(USB_VideoStreamsTask, "usb_streams", 2500L / sizeof(portSTACK_TYPE), (void *)g_Cameras, 3, NULL) != pdPASS) {
        usb_echo("create usb stream task error\r\n");
    }    

    if (xTaskCreate(USB_HostVideoFramesTask, "usb_frames", 1024L / sizeof(portSTACK_TYPE),NULL, configMAX_PRIORITIES - 2, NULL) != pdPASS) {
        usb_echo("create usb frames task error\r\n");
    }
}
