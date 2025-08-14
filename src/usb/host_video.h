/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HOST_VIDEO_H_
#define _HOST_VIDEO_H_

// clang-format off
#include "usb_host_config.h"
#include "usb_host.h"
#include "fsl_device_registers.h"
#include "usb_host_video.h"
#include "protocol/protocol.h"
// clang-format on

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define FULL_SPEED_ISO_MAX_PACKET_SIZE               (1023U)
#define HIGH_SPEED_ISO_MAX_PACKET_SIZE_ZERO_ADDITION (1024U)

#define USB_VIDEO_STREAM_BUFFER_COUNT \
    (6U) /*!< the prime count, the value shouldn't be more than 6 because of USB dedicated ram limination */


#ifdef NETWORK_SINGLE_INTERFACE
#define MAX_FRAME_BUFFER 2 // 2 cameras, 2 buffers each
#define MAX_FRAME_SIZE  128 * 1024
#else
#define MAX_FRAME_BUFFER 4 // 4 cameras, 2 buffers each
#define MAX_FRAME_SIZE  64 * 1024
#endif

#define MAX_USB_CAMERAS 2
#define CAMERA_SWITCH_STREAM_TIMEOUT 1000 // ms
typedef struct frame_data {
    uint8_t slot;
    uint8_t idx;
    uint8_t pictureBufferIndex;
    uint8_t frameId;
    uint8_t *frame[2];
    uint32_t size[2];
    uint8_t ready;
    uint8_t is_ext;
    uint8_t address[2];
    uint32_t last_time;
} frame_data_t;

typedef enum _usb_host_app_state {
    kStatus_DEV_Idle = 0, /*!< there is no device attach/detach */
    kStatus_DEV_Attached, /*!< device is attached */
    kStatus_DEV_Detached, /*!< device is detached */
    kStatus_DEV_Restart , /*!< device is restarted */
} usb_host_app_state_t;

typedef enum _usb_host_video_run_state {
    kUSB_HostVideoRunIdle = 0,                /*!< idle */
    kUSB_HostVideoRunSetControlInterface,     /*!< execute set control interface code */
    kUSB_HostVideoRunSetControlInterfaceDone, /*!< set control interface done */
    kUSB_HostVideoRunWaitSetControlInterface, /*!< wait control interface done */
    kUSB_HostVideoRunWaitSetStreamInterface,  /*!< wait steam interface done */
    kUSB_HostVideoRunSetInterfaceDone,        /*!< set interface done */
    kUSB_HostVideoRunFindOptimalSetting,      /*!< find optimal setting */
    kUSB_HostVideoRunGetSetProbeCommit,       /*!< set or get probe commit start*/
    kUSB_HostVideoRunWaitGetSetProbeCommit,   /*!< wait set or get probe commit done */
    kUSB_HostVideoRunGetSetProbeCommitDone,   /*!< set or get probe commit done */
    kUSB_HostVideoStart,                      /*!< start video camera */
    kUSB_HostVideoRunVideoDone,               /*!< video done */
} usb_host_video_camera_run_state_t;

typedef struct _usb_host_video_camera_instance {
    usb_device_handle deviceHandle;              /*!< the video camera device handle */
    usb_host_class_handle classHandle;           /*!< the video camera class handle */
    usb_host_interface_handle controlIntfHandle; /*!< the video camera control interface handle */
    usb_host_interface_handle streamIntfHandle;  /*!< the video camera stream interface handle */
    usb_host_video_stream_payload_format_common_desc_t
        *videoStreamFormatDescriptor; /*!< the video camera stream common format descriptor pointer */
    usb_host_video_stream_payload_mjpeg_format_desc_t
        *videoStreamMjpegFormatDescriptor; /*!< the video camera stream mjpeg format descriptor pointer */
    usb_host_video_stream_payload_uncompressed_format_desc_t
        *videoStreamUncompressedFormatDescriptor; /*!< the video camera stream uncompressed format descriptor pointer */
    usb_host_video_stream_payload_frame_common_desc_t
        *videoStreamFrameDescriptor; /*!< the video camera stream common frame descriptor pointer */
    usb_host_video_stream_payload_mjpeg_frame_desc_t
        *videoStreamMjpegFrameDescriptor; /*!< the video camera stream mjpeg frame descriptor pointer */
    usb_host_video_stream_payload_uncompressed_frame_desc_t
        *videoStreamUncompressedFrameDescriptor; /*!< the video camera stream uncompressed frame descriptor pointer */

    uint16_t unMultipleIsoMaxPacketSize;     /*!< the packet size of using iso endpoint (<=1024)*/
    uint8_t cameraDeviceFormatType;          /*!< the current device format type */
    uint8_t devState;                        /*!< the device attach/detach status */
    uint8_t prevState;                       /*!< the device attach/detach previous status */
    uint8_t runState;                        /*!< the video camera application run status */
    uint8_t runWaitState;                    /*!< the video camera application run wait status */
    volatile uint8_t isControlTransferring;  /*!< the control transfer is processing */
    uint8_t currentStreamInterfaceAlternate; /*!< the alternate of stream interface that include used endpoint */
    volatile uint8_t streamBufferIndex;      /*!< the prime buffer index */
    volatile uint8_t pictureBufferIndex;      /*!< the prime buffer index */
    uint8_t step;

    int32_t errors; /*!< the error count */
    uint8_t expect_frame_id;
    uint8_t port;
    uint8_t g_index; // index of the camera in global array
    volatile uint8_t stream_id; // index of the stream

} usb_host_video_camera_instance_t;


extern void USB_HostVideoInitialize(void);
extern void USB_HostStreamVideoTask(void *param);
extern void USB_HostVideoFramesTask(void *param);
extern usb_status_t USB_HostVideoEvent(usb_device_handle deviceHandle,
                                       usb_host_configuration_handle configurationHandle,
                                       uint32_t eventCode);
extern void toggle_cam(uint8_t code);

#endif /* _HOST_VIDEO_H_ */
