/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// #include "usb_host_config.h"
// #include "usb_host.h"

#include "host_video.h"
#include "platform/platform.h"
#include "usb_host_hci.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief usb host video command complete callback.
 *
 * This function is used as callback function for completed command.
 *
 * @param param    the host video instance pointer.
 * @param data      data buffer pointer.
 * @param dataLen data length.
 * @param status         transfer result status.
 */
static void USB_HostVideoCommandCallback(void *param, uint8_t *data, uint32_t dataLen, usb_status_t status);

/*!
 * @brief usb host video control transfer callback.
 *
 * This function is used as callback function for control transfer .
 *
 * @param param    the host video instance pointer.
 * @param data      data buffer pointer.
 * @param dataLen data length.
 * @param status         transfer result status.
 */
static void USB_HostVideoControlCallback(void *param, uint8_t *data, uint32_t dataLen, usb_status_t status);

/*!
 * @brief host video stream iso in transfer callback.
 *
 * This function is used as callback function when call USB_HosVideoStreamRecv .
 *
 * @param param    the host video instance pointer.
 * @param data      data buffer pointer.
 * @param dataLen data length.
 * @param status    transfer result status.
 */
static void USB_HostVideoStreamDataInCallback(void *param, uint8_t *data, uint32_t dataLen, usb_status_t status);

/* Helper function to check if an interface supports MJPEG format */
static usb_status_t USB_HostVideoCheckForMJPEG(usb_host_interface_t *hostInterface);

/*******************************************************************************
 * Variables
 ******************************************************************************/

usb_host_video_camera_instance_t g_Cameras[MAX_USB_CAMERAS];//

usb_device_handle g_VideoDeviceHandle[MAX_USB_CAMERAS];
usb_host_interface_handle g_VideoStreamInterfaceHandle[MAX_USB_CAMERAS];
usb_host_interface_handle g_VideoControlInterfaceHandle[MAX_USB_CAMERAS];

static volatile int i_Cameras = 0;
static usb_host_video_camera_instance_t *port_cameras[5];

USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
static usb_host_video_probe_commit_controls_t s_VideoProbe[MAX_USB_CAMERAS];
/* probe buffer to transfer */
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
static usb_host_video_probe_commit_controls_t s_Probe[MAX_USB_CAMERAS][3];
/* usb video stream transfer buffer */
USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
static uint8_t s_streamBuffer[MAX_USB_CAMERAS][USB_VIDEO_STREAM_BUFFER_COUNT][HIGH_SPEED_ISO_MAX_PACKET_SIZE_ZERO_ADDITION];

USB_DMA_NONINIT_DATA_ALIGN(USB_DATA_ALIGN_SIZE)
frame_data_t frames[4];

/* GUID, Globally Unique Identifier used to identify stream-encoding format */
const uint8_t g_YUY2FormatGUID[] = {0x59, 0x55, 0x59, 0x32, 0x00, 0x00, 0x10, 0x00,
                                    0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};
const uint8_t g_NV12FormatGUID[] = {0x4E, 0x56, 0x31, 0x32, 0x00, 0x00, 0x10, 0x00,
                                    0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};
const uint8_t g_M420FormatGUID[] = {0x4D, 0x34, 0x32, 0x30, 0x00, 0x00, 0x10, 0x00,
                                    0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};
const uint8_t g_I420FormatGUID[] = {0x49, 0x34, 0x32, 0x30, 0x00, 0x00, 0x10, 0x00,
                                    0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};

__attribute__((section(".UsbPoolSection"))) __attribute__((aligned(4)))
static uint8_t frameBuffer[MAX_FRAME_BUFFER][2][MAX_FRAME_SIZE] = {0};
    
void USB_HostVideoInitialize() {
    for(int i = 0; i < MAX_FRAME_BUFFER; i++) {
        frames[i].frame[0] = &(frameBuffer[i][0][0]);
        frames[i].frame[1] = &(frameBuffer[i][1][0]);
        frames[i].is_ext = 0;
    }
}

/*!
 * @brief get proper endpoint maxpacksize and the subordinate interface alternate.
 *
 * This function is used select proper interface alternate setting according to the expected max packet size
 *
 * @param classHandle   the class handle.
 * @param expectMaxPacketSize   the expected max packet size
 * @param interfaceAlternate   the pointer to the selected interface alternate setting
 * @param unMultipleIsoPacketSize  the actually used endpoint max packet size
 *
 * @retval kStatus_USB_Success        successfully get the proper interface aternate setting
 * @retval kStatus_USB_InvalidHandle  The streamInterface is NULL pointer.
 * @retval kStatus_USB_Error          There is no proper interface alternate
 */

static uint8_t USB_HostGetProperEndpointInterfaceInfo(usb_host_class_handle classHandle,
                                                      uint16_t expectMaxPacketSize,
                                                      uint8_t *interfaceAlternate,
                                                      uint16_t *unMultipleIsoPacketSize) {
    usb_host_video_instance_struct_t *videoInstance = (usb_host_video_instance_struct_t *)classHandle;
    usb_host_interface_t *streamInterface;
    streamInterface = (usb_host_interface_t *)videoInstance->streamIntfHandle;
    usb_host_video_descriptor_union_t descriptor;
    uint8_t epCount   = 0;
    uint16_t length   = 0;
    uint8_t alternate = 0;

    if (NULL == streamInterface) {
        return kStatus_USB_InvalidHandle;
    }
    if (NULL == streamInterface->interfaceDesc) {
        return kStatus_USB_InvalidParameter;
    }

    descriptor.bufr = streamInterface->interfaceExtension;
    length          = 0;
    while (length < streamInterface->interfaceExtensionLength) {
        if (descriptor.common->bDescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE) {
            break;
        }
        length += descriptor.common->bLength;
        descriptor.bufr += descriptor.common->bLength;
    }

    while (length < streamInterface->interfaceExtensionLength) {
        if (descriptor.common->bDescriptorType == USB_DESCRIPTOR_TYPE_INTERFACE) {
            alternate = descriptor.interface->bAlternateSetting;
            epCount   = descriptor.interface->bNumEndpoints;
            while (epCount) {
                if (descriptor.endpoint->bDescriptorType == USB_DESCRIPTOR_TYPE_ENDPOINT) {
                    uint16_t maxPacketSize = USB_SHORT_FROM_LITTLE_ENDIAN_DATA(descriptor.endpoint->wMaxPacketSize);
                    if ((maxPacketSize <= expectMaxPacketSize) && (*unMultipleIsoPacketSize < maxPacketSize)) {
                        *unMultipleIsoPacketSize = maxPacketSize;
                        /* save interface alternate for the current proper endpoint */
                        *interfaceAlternate = alternate;
                    }
                    epCount--;
                }
                length += descriptor.common->bLength;
                descriptor.bufr += descriptor.common->bLength;
            }
        } else {
            length += descriptor.common->bLength;
            descriptor.bufr += descriptor.common->bLength;
        }
    }
    if ((0 == *unMultipleIsoPacketSize) || (0 == *interfaceAlternate)) {
        return kStatus_USB_Error;
    } else {
        return kStatus_USB_Success;
    }
}

/*!
 * @brief usb host video command complete callback.
 *
 * This function is used as callback function for completed command.
 *
 * @param param    the host video instance pointer.
 * @param data      data buffer pointer.
 * @param dataLen data length.
 * @param status         transfer result status.
 */

static void USB_HostVideoCommandCallback(void *param, uint8_t *data, uint32_t dataLen, usb_status_t status) {
    usb_host_video_camera_instance_t *videoInstance = (usb_host_video_camera_instance_t *)param;
    videoInstance->isControlTransferring            = 0;
}

/*!
 * @brief usb host video control transfer callback.
 *
 * This function is used as callback function for control transfer .
 *
 * @param param    the host video instance pointer.
 * @param data      data buffer pointer.
 * @param dataLen data length.
 * @param status         transfer result status.
 */

static void USB_HostVideoControlCallback(void *param, uint8_t *data, uint32_t dataLen, usb_status_t status) {
    usb_host_video_camera_instance_t *videoInstance = (usb_host_video_camera_instance_t *)param;
    if (status != kStatus_USB_Success) {
        usb_echo("data transfer error = %d , status \r\n");
        return;
    }

    if (videoInstance->runState == kUSB_HostVideoRunIdle) {
        if (videoInstance->runWaitState == kUSB_HostVideoRunSetControlInterface) {
            videoInstance->runState = kUSB_HostVideoRunSetControlInterfaceDone;
        } else if (videoInstance->runWaitState == kUSB_HostVideoRunWaitSetStreamInterface) {
            videoInstance->runState = kUSB_HostVideoRunSetInterfaceDone;
        } else if (videoInstance->runWaitState == kUSB_HostVideoRunWaitGetSetProbeCommit) {
            videoInstance->runState = kUSB_HostVideoRunGetSetProbeCommitDone;
        }
    }
}

static usb_status_t USB_HostVideoStreamStop(usb_host_class_handle classHandle, usb_host_interface_handle interfaceHandle) {
    usb_host_video_instance_struct_t *videoInstance = (usb_host_video_instance_struct_t *)classHandle;
    usb_host_transfer_t *transfer;
    usb_status_t status;

    if ((classHandle == NULL) || (interfaceHandle == NULL))
    {
        return kStatus_USB_InvalidParameter;
    }

    if (USB_HostMallocTransfer(videoInstance->hostHandle, &transfer) != kStatus_USB_Success)
    {
        return kStatus_USB_Error;
    }

    transfer->setupPacket->bRequest      = USB_REQUEST_STANDARD_SET_INTERFACE;
    transfer->setupPacket->bmRequestType = USB_REQUEST_TYPE_RECIPIENT_INTERFACE;
    transfer->setupPacket->wIndex        = USB_SHORT_TO_LITTLE_ENDIAN(
        ((usb_host_interface_t *)interfaceHandle)->interfaceDesc->bInterfaceNumber);
    transfer->setupPacket->wValue        = USB_SHORT_TO_LITTLE_ENDIAN(0);
    transfer->setupPacket->wLength       = 0;
    transfer->transferBuffer             = NULL;
    transfer->transferLength             = 0;

    status = USB_HostSendSetup(videoInstance->hostHandle, videoInstance->controlPipe, transfer);

    if (status == kStatus_USB_Success)
    {
        videoInstance->controlTransfer = transfer;
    }
    else
    {
        USB_HostFreeTransfer(videoInstance->hostHandle, transfer);
    }

    return status;
}


int check_frame(uint8_t idx) {
    uint8_t buffIdx = frames[idx].pictureBufferIndex; 
    uint32_t size = frames[idx].size[buffIdx];
    uint8_t *frame = frames[idx].frame[buffIdx];

    if (size < 4) {
        return 0; // not enough data to check
    }

    if (frame[0] != 0xFF || frame[1] != 0xD8) {
        return 0; // JPEG SOI not found
    }

    for (int i = size - 2; i >= 0; i--) {
        if (frame[i] == 0xFF && frame[i + 1] == 0xD9) {
            return i + 2;
        }
    }

    return 0; // JPEG EOI not found
}

/*!
 * @brief host video stream iso in transfer callback.
 *
 * This function is used as callback function when call USB_HosVideoStreamRecv .
 *
 * @param param    the host video instance pointer.
 * @param data      data buffer pointer.
 * @param dataLen data length.
 * @param status    transfer result status.
 */

static void USB_HostVideoStreamDataInCallback(void *param, uint8_t *data, uint32_t dataLen, usb_status_t status) {
    
    usb_host_video_camera_instance_t *camera = (usb_host_video_camera_instance_t *)param;
    uint8_t idx                              = camera->g_index;
    uint32_t headLength;
    uint32_t dataSize;
    uint8_t endOfFrame;
    uint8_t frameId;
    uint32_t presentationTime;
    uint8_t error;

    if (camera->devState != kStatus_DEV_Attached) {
        return;
    }

    usb_host_video_payload_header_t *header =
        ((usb_host_video_payload_header_t *)s_streamBuffer[idx][camera->streamBufferIndex]);

    endOfFrame       = header->HeaderInfo.bitMap.end_of_frame;
    frameId          = header->HeaderInfo.bitMap.frame_id;
    presentationTime = USB_LONG_FROM_LITTLE_ENDIAN_ADDRESS(header->dwPresentationTime);
    headLength       = header->bHeaderLength;
    error            = header->HeaderInfo.bitMap.error;

    if (dataLen > 0) {
        /* the standard header is 12 bytes */
        if (dataLen > 11) {
            dataSize = dataLen - headLength;
            /* there is payload for this transfer */
            if (dataSize) {
                if (camera->expect_frame_id == 0xFF) {
                    camera->expect_frame_id = frameId;
                    camera->errors          = 0;
                    frames[idx].size[camera->pictureBufferIndex] = 0;
                }
                
                if (camera->expect_frame_id == frameId) {
                    if (error) {
                        camera->errors++;
                    }
                    
                    if (camera->errors == 0) {
                        uint8_t *payload = (((uint8_t *)&s_streamBuffer[idx][camera->streamBufferIndex][0]) + headLength);
                        if(frames[idx].size[camera->pictureBufferIndex] + dataSize <= MAX_FRAME_SIZE) {
                            uint8_t *dest = frames[idx].frame[camera->pictureBufferIndex] + frames[idx].size[camera->pictureBufferIndex];
                            memcpy(dest, payload, dataSize);
                            frames[idx].size[camera->pictureBufferIndex] += dataSize;
                        } else {
                            camera->errors++;
                        }
                    }
                }
            }

            // Datasize could be 0 if the header is the last one in the frame
            if (endOfFrame) {
                uint8_t slot = (camera->port - 1) * 2;
                frames[idx].frameId            = frameId;
                frames[idx].idx                = idx;
                frames[idx].slot               = slot;
                frames[idx].pictureBufferIndex = camera->pictureBufferIndex;
                frames[idx].ready              = camera->errors > 0 ? 0 : 1;

                camera->pictureBufferIndex = (camera->pictureBufferIndex + 1) % 2;
                camera->expect_frame_id = 0xFF;
            }
        }
    }

    /* prime transfer for this usb stream buffer again */
    USB_HosVideoStreamRecv(
        camera->classHandle, (uint8_t *)&s_streamBuffer[idx][camera->streamBufferIndex][0],
        camera->unMultipleIsoMaxPacketSize, USB_HostVideoStreamDataInCallback, camera);
    

    camera->streamBufferIndex++;
    if (camera->streamBufferIndex == USB_VIDEO_STREAM_BUFFER_COUNT) {
        camera->streamBufferIndex = 0;
    }
}

extern usb_host_handle g_HostHandle;
volatile uint8_t camera_stream_id = 0;

void toggle_cam(uint8_t code){
    static uint32_t last_camera_stream_toggle = 0;
    static uint8_t last_cam_state_switch = 0;

    if (code && !last_cam_state_switch) {
        uint32_t now = get_time();
        // Force minimum time between toggles
        if (now - last_camera_stream_toggle >= CAMERA_SWITCH_STREAM_TIMEOUT) {
            camera_stream_id = (camera_stream_id + 1) % 2;
            last_camera_stream_toggle = now;
        }
    }

    last_cam_state_switch = code;
}

void USB_HostStreamVideoTask(void *param) {
    usb_status_t status                                          = kStatus_USB_Success;
    usb_host_video_stream_payload_frame_common_desc_t *frameDesc = NULL;
    usb_host_video_camera_instance_t *camera                     = (usb_host_video_camera_instance_t *)param;

    uint16_t wWitd, wHeight;
    uint32_t index, count;
    uint32_t frameInterval = 0;
    uint32_t speed         = 0U;
    uint8_t i              = 0;
    uint8_t idx            = camera->g_index;
    int8_t  stream_id      = camera_stream_id;

    if (platform_state != PLATFORM_STATE_RUNNING && camera->devState == kStatus_DEV_Attached) {
        if (camera->runState == kUSB_HostVideoRunIdle ||
            camera->runState == kUSB_HostVideoStart ||
            camera->runState == kUSB_HostVideoRunGetSetProbeCommit) {
            camera->devState = kStatus_DEV_Detached;
        }
    }

    if (camera->stream_id != stream_id && camera->devState == kStatus_DEV_Attached) {
        if (camera->runState == kUSB_HostVideoRunIdle ||
            camera->runState == kUSB_HostVideoStart ||
            camera->runState == kUSB_HostVideoRunGetSetProbeCommit) {
            camera->stream_id = stream_id;
            camera->devState = kStatus_DEV_Restart;
        }
    }

    /* device state changes */
    if (camera->devState != camera->prevState) {
        camera->prevState = camera->devState;
        switch (camera->devState) {
            case kStatus_DEV_Idle:
                break;

            case kStatus_DEV_Attached:
                camera->runState                        = kUSB_HostVideoRunSetControlInterface;
                camera->isControlTransferring           = 0;
                camera->streamBufferIndex               = 0;
                camera->cameraDeviceFormatType          = 0;
                camera->unMultipleIsoMaxPacketSize      = 0;
                camera->currentStreamInterfaceAlternate = 0;
                camera->step                            = 0;
                camera->expect_frame_id                 = 0xFF;
                camera->errors                          = 0;
                USB_HostVideoInit(camera->deviceHandle, &camera->classHandle);
                usb_echo("USB video attached\r\n");
                break;

            case kStatus_DEV_Detached:
                camera->devState     = kStatus_DEV_Idle;
                camera->runState     = kUSB_HostVideoRunIdle;
                camera->runWaitState = kStatus_DEV_Idle;
                USB_HostVideoDeinit(camera->deviceHandle, camera->classHandle);
                camera->classHandle = NULL;
                usb_echo("USB video detached\r\n");
                break;

            case kStatus_DEV_Restart: {
                OSA_SR_ALLOC();
                OSA_ENTER_CRITICAL();

                // // Stop the stream if it is running
                if (camera->currentStreamInterfaceAlternate != 0) {
                    USB_HostVideoStreamStop(camera->classHandle, camera->streamIntfHandle);
                    // TODO: runState with semaphore or to wait for the stream to stop instead of delay
                    vTaskDelay(pdMS_TO_TICKS(30));
                    usb_echo("Stopping stream...\r\n");
                }

                // Deinitialize the camera
                USB_HostVideoDeinit(camera->deviceHandle, camera->classHandle);
                camera->classHandle                     = NULL;
                usb_echo("USB video deinitialized\r\n");

                // Reinitialize the camera
                camera->devState                        = kStatus_DEV_Attached;
                camera->prevState                       = kStatus_DEV_Attached;
                camera->runWaitState                    = kStatus_DEV_Idle;
                camera->runState                        = kUSB_HostVideoRunSetControlInterface;
                camera->isControlTransferring           = 0;
                camera->streamBufferIndex               = 0;
                camera->cameraDeviceFormatType          = 0;
                camera->unMultipleIsoMaxPacketSize      = 0;
                camera->currentStreamInterfaceAlternate = 0;
                camera->step                            = 0;
                camera->expect_frame_id                 = 0xFF;
                camera->errors                          = 0;
                USB_HostVideoInit(camera->deviceHandle, &camera->classHandle);
                usb_echo("USB video reinitialized\r\n");
                OSA_EXIT_CRITICAL();
                break;
            }
            default:
                break;
        }
    }

    /* run state */
    switch (camera->runState) {
        case kUSB_HostVideoRunIdle:
            vTaskDelay(pdMS_TO_TICKS(10));
            break;

        case kUSB_HostVideoRunSetControlInterface:
            camera->runWaitState = kUSB_HostVideoRunSetControlInterface;
            camera->runState     = kUSB_HostVideoRunIdle;
            if (USB_HostVideoControlSetInterface(camera->classHandle, camera->controlIntfHandle, 0,
                                                 USB_HostVideoControlCallback, camera) != kStatus_USB_Success) {
                usb_echo("set interface error\r\n");
            }
            break;
        case kUSB_HostVideoRunSetControlInterfaceDone:
            camera->runWaitState                    = kUSB_HostVideoRunWaitSetStreamInterface;
            camera->runState                        = kUSB_HostVideoRunIdle;
            camera->currentStreamInterfaceAlternate = 0;
            if (USB_HostVideoStreamSetInterface(camera->classHandle, camera->streamIntfHandle,
                                                camera->currentStreamInterfaceAlternate,
                                                USB_HostVideoControlCallback, camera) != kStatus_USB_Success) {
                usb_echo("set interface error\r\n");
            }
            break;
        case kUSB_HostVideoRunSetInterfaceDone:
            camera->runState = kUSB_HostVideoRunFindOptimalSetting;
            break;
        case kUSB_HostVideoRunFindOptimalSetting:
            /* Firstly get MJPEG format descriptor */
            status = USB_HostVideoStreamGetFormatDescriptor(camera->classHandle,
                                                            USB_HOST_DESC_SUBTYPE_VS_FORMAT_MJPEG,
                                                            (void *)&camera->videoStreamFormatDescriptor);
            if (status == kStatus_USB_InvalidHandle) {
                usb_echo("camera->classHandle is invalid\r\n");
            } else if (status == kStatus_USB_Error) {
                /* the camera device doesn't support MJPEG format, try to get UNCOMPRESSED format */
                status = USB_HostVideoStreamGetFormatDescriptor(camera->classHandle,
                                                                USB_HOST_DESC_SUBTYPE_VS_FORMAT_UNCOMPRESSED,
                                                                (void *)&camera->videoStreamFormatDescriptor);
                if (status == kStatus_USB_InvalidHandle) {
                    usb_echo("camera->classHandle is invalid\r\n");
                } else if (status == kStatus_USB_Error) {
                    usb_echo(" host can't support this format camera device\r\n");
                    camera->runState = kUSB_HostVideoRunIdle;
                } else {
                }
            } else {
            }

            /* Successfully get MJPEG or UNCOMPRESSED format descriptor */
            if (status == kStatus_USB_Success) {
                count = camera->videoStreamFormatDescriptor->bNumFrameDescriptors;
                camera->cameraDeviceFormatType =
                    camera->videoStreamFormatDescriptor->bDescriptorSubtype;
                if (camera->cameraDeviceFormatType ==
                    USB_HOST_DESC_SUBTYPE_VS_FORMAT_MJPEG) /* camera device supports mjpeg format */
                {
                    camera->videoStreamMjpegFormatDescriptor =
                        (usb_host_video_stream_payload_mjpeg_format_desc_t *)
                            camera->videoStreamFormatDescriptor;
                } else if (camera->cameraDeviceFormatType ==
                           USB_HOST_DESC_SUBTYPE_VS_FORMAT_UNCOMPRESSED) /* camera device supports uncompressed format */
                {
                    camera->videoStreamUncompressedFormatDescriptor =
                        (usb_host_video_stream_payload_uncompressed_format_desc_t *)
                            camera->videoStreamFormatDescriptor;
                }

                for (index = 1; index <= count; index++) {
                    if (camera->cameraDeviceFormatType == USB_HOST_DESC_SUBTYPE_VS_FORMAT_MJPEG) {
                        status = USB_HostVideoStreamGetFrameDescriptor(
                            camera->classHandle, camera->videoStreamFormatDescriptor,
                            USB_HOST_DESC_SUBTYPE_VS_FRAME_MJPEG, index, (void *)&frameDesc);

                        if ((kStatus_USB_Success != status) || (NULL == frameDesc)) {
                            continue;
                        }

                        wWitd   = (uint16_t)(USB_SHORT_FROM_LITTLE_ENDIAN_ADDRESS(frameDesc->wWitd));
                        wHeight = (uint16_t)(USB_SHORT_FROM_LITTLE_ENDIAN_ADDRESS(frameDesc->wHeight));

                        if (camera->stream_id == 1) {
                            if ((wWitd != 640) || (wHeight != 472)) {
                                continue;
                            }
                        } else {
                            if ((wWitd != 640) || (wHeight != 480)) {
                                continue;
                            }
                        }
                        
                        camera->videoStreamMjpegFrameDescriptor =
                            (usb_host_video_stream_payload_mjpeg_frame_desc_t *)frameDesc;
                        camera->videoStreamUncompressedFrameDescriptor = NULL;
                        break;
                    }

                    /** TODO: add support for thermal camera */
                    // if (camera->cameraDeviceFormatType == USB_HOST_DESC_SUBTYPE_VS_FORMAT_UNCOMPRESSED) {
                    //     status = USB_HostVideoStreamGetFrameDescriptor(
                    //         camera->classHandle, camera->videoStreamFormatDescriptor,
                    //         USB_HOST_DESC_SUBTYPE_VS_FRAME_UNCOMPRESSED, index, (void *)&frameDesc);
                    //     if ((kStatus_USB_Success == status) && (NULL != frameDesc)) {
                    //         camera->videoStreamUncompressedFrameDescriptor =
                    //             (usb_host_video_stream_payload_uncompressed_frame_desc_t *)frameDesc;
                    //         camera->videoStreamMjpegFrameDescriptor = NULL;
                    //     }
                    // }
                }

                /* successfully get frame descriptor that has a minimum resolution, or go into idle state */
                if (index <= count) {
                    camera->runState = kUSB_HostVideoRunGetSetProbeCommit;
                    camera->step     = 0;
                } else {
                    status           = kStatus_USB_Error;
                    camera->runState = kUSB_HostVideoRunIdle;
                }
            }
            break;
        case kUSB_HostVideoRunGetSetProbeCommit:
            switch (camera->step) {
                case 0:
                    /* get the current setting of device camera */
                    if (camera->isControlTransferring == 0) {
                        camera->isControlTransferring = 1;
                        status                        = USB_HostVideoGetProbe(camera->classHandle, USB_HOST_VIDEO_GET_CUR,
                                                                              (void *)&s_Probe[idx][0], USB_HostVideoCommandCallback, camera);
                        camera->step++;
                    }
                    break;
                    /* the maximum setting for device camera */
                case 1:
                    if (camera->isControlTransferring == 0) {
                        camera->isControlTransferring = 1;
                        status                        = USB_HostVideoGetProbe(camera->classHandle, USB_HOST_VIDEO_GET_MAX,
                                                                              (void *)&s_Probe[idx][1], USB_HostVideoCommandCallback, camera);
                        camera->step++;
                    }
                    break;
                case 2:
                    /* the minimum value for device camera */
                    if (camera->isControlTransferring == 0) {
                        camera->isControlTransferring = 1;
                        status                        = USB_HostVideoGetProbe(camera->classHandle, USB_HOST_VIDEO_GET_MIN,
                                                                              (void *)&s_Probe[idx][2], USB_HostVideoCommandCallback, camera);
                        camera->step++;
                    }
                    break;
                case 3:
                    /* the seleted frame can support multiple interval, choose the maximum one */
                    if (camera->isControlTransferring == 0) {
                        camera->isControlTransferring = 1;
                        if (camera->cameraDeviceFormatType == USB_HOST_DESC_SUBTYPE_VS_FORMAT_MJPEG) {
                            if (camera->videoStreamMjpegFrameDescriptor->bFrameIntervalType > 0) {
                                frameInterval = USB_LONG_FROM_LITTLE_ENDIAN_ADDRESS(
                                    ((uint8_t *)&(
                                         camera->videoStreamMjpegFrameDescriptor->dwMinFrameInterval[0]) +
                                     (camera->videoStreamMjpegFrameDescriptor->bFrameIntervalType - 1) * 4));
                            } else {
                                frameInterval = USB_LONG_FROM_LITTLE_ENDIAN_ADDRESS(
                                    ((uint8_t *)&(
                                         camera->videoStreamMjpegFrameDescriptor->dwMinFrameInterval[0]) +
                                     4));
                            }
                           
                            /* save the frame interval, frame index and format index */
                            USB_LONG_TO_LITTLE_ENDIAN_ADDRESS(frameInterval, s_VideoProbe[idx].dwFrameInterval);
                            s_VideoProbe[idx].bFormatIndex =
                                camera->videoStreamMjpegFormatDescriptor->bFormatIndex;
                            s_VideoProbe[idx].bFrameIndex = camera->videoStreamMjpegFrameDescriptor->bFrameIndex;
                            
                        } else if (camera->cameraDeviceFormatType ==
                                   USB_HOST_DESC_SUBTYPE_VS_FORMAT_UNCOMPRESSED) {
                            if (camera->videoStreamUncompressedFrameDescriptor->bFrameIntervalType > 0) {
                                frameInterval = USB_LONG_FROM_LITTLE_ENDIAN_ADDRESS((
                                    (uint8_t *)&(camera->videoStreamUncompressedFrameDescriptor
                                                     ->dwMinFrameInterval[0]) +
                                    (camera->videoStreamUncompressedFrameDescriptor->bFrameIntervalType - 1) *
                                        4));
                            } else {
                                frameInterval = USB_LONG_FROM_LITTLE_ENDIAN_ADDRESS(
                                    ((uint8_t *)&(camera->videoStreamUncompressedFrameDescriptor
                                                      ->dwMinFrameInterval[0]) +
                                     4));
                            }

                            /* save the frame interval, frame index and format index */
                            USB_LONG_TO_LITTLE_ENDIAN_DATA(frameInterval, s_VideoProbe[idx].dwFrameInterval);
                            
                            s_VideoProbe[idx].bFormatIndex =
                                camera->videoStreamUncompressedFormatDescriptor->bFormatIndex;
                            s_VideoProbe[idx].bFrameIndex =
                                camera->videoStreamUncompressedFrameDescriptor->bFrameIndex;
                        }
                        camera->isControlTransferring = 1;
                        /* set device camera using the new probe */
                        status = USB_HostVideoSetProbe(camera->classHandle, USB_HOST_VIDEO_SET_CUR,
                                                       (void *)&s_VideoProbe[idx], USB_HostVideoCommandCallback, camera);
                        camera->step++;
                    }
                    break;
                case 4:
                case 5:
                case 6:
                    /* get the current/min/max state of device camera */
                    if (camera->isControlTransferring == 0) {
                        camera->isControlTransferring = 1;
                        if (camera->step == 4) {
                            status = USB_HostVideoGetProbe(camera->classHandle, USB_HOST_VIDEO_GET_CUR,
                                                           (void *)&s_Probe[idx][0], USB_HostVideoCommandCallback, camera);
                        } else if (camera->step == 5) {
                            status = USB_HostVideoGetProbe(camera->classHandle, USB_HOST_VIDEO_GET_MAX,
                                                           (void *)&s_Probe[idx][1], USB_HostVideoCommandCallback, camera);
                        } else {
                            status = USB_HostVideoGetProbe(camera->classHandle, USB_HOST_VIDEO_GET_MIN,
                                                           (void *)&s_Probe[idx][2], USB_HostVideoCommandCallback, camera);
                        }
                        camera->step++;
                    }
                    break;
                case 7:
                    /* do multiple get/set cur requests to make sure a stable setting is obtained */
                    if (camera->isControlTransferring == 0) {
                        camera->isControlTransferring = 1;
                        memcpy((void *)&s_VideoProbe[idx], (void *)&s_Probe[idx][0], 22);
                        status = USB_HostVideoSetProbe(camera->classHandle, USB_HOST_VIDEO_SET_CUR,
                                                       (void *)&s_VideoProbe[idx], USB_HostVideoCommandCallback, camera);
                        camera->step++;
                    }
                    break;
                case 8:
                    if (camera->isControlTransferring == 0) {
                        camera->isControlTransferring = 1;
                        status                        = USB_HostVideoGetProbe(camera->classHandle, USB_HOST_VIDEO_GET_CUR,
                                                                              (void *)&s_Probe[idx][0], USB_HostVideoCommandCallback, camera);
                        camera->step++;
                    }
                    break;
                case 9:
                    if (camera->isControlTransferring == 0) {
                        camera->isControlTransferring = 1;
                        memcpy((void *)&s_VideoProbe[idx], (void *)&s_Probe[idx][0], 26);
                        status = USB_HostVideoSetProbe(camera->classHandle, USB_HOST_VIDEO_SET_CUR,
                                                       (void *)&s_VideoProbe[idx], USB_HostVideoCommandCallback, camera);
                        camera->step++;
                    }
                    break;
                case 10:
                    if (camera->isControlTransferring == 0) {
                        camera->isControlTransferring = 1;
                        status                        = USB_HostVideoGetProbe(camera->classHandle, USB_HOST_VIDEO_GET_CUR,
                                                                              (void *)&s_Probe[idx][0], USB_HostVideoCommandCallback, camera);
                        camera->step++;
                    }
                    break;
                case 11:
                    /* configure the hardware with the negotiated parameters */
                    if (camera->isControlTransferring == 0) {
                        camera->isControlTransferring = 1;
                        memcpy((void *)&s_VideoProbe[idx], (void *)&s_Probe[idx][0], 26);
                        /* delay 20ms to make sure device is ready and then configure actually */
                        vTaskDelay(pdMS_TO_TICKS(20));
                        status = USB_HostVideoSetCommit(camera->classHandle, USB_HOST_VIDEO_SET_CUR,
                                                        (void *)&s_VideoProbe[idx], USB_HostVideoCommandCallback, camera);
                        camera->step++;
                    }
                    break;
                case 12:
                    if (camera->isControlTransferring == 0) {
                        USB_HostHelperGetPeripheralInformation(camera->deviceHandle, kUSB_HostGetDeviceSpeed,
                                                               &speed);
                        /* According to the device speed mode, choose a proper video stream interface */
                        camera->unMultipleIsoMaxPacketSize = 0;
                        if (USB_SPEED_FULL == speed) {
                            /* if device is full speed, the selected endpoint's maxPacketSize is maximum device supports
                             * and is not more than 1023B */    
                            USB_HostGetProperEndpointInterfaceInfo(camera->classHandle,
                                                                   FULL_SPEED_ISO_MAX_PACKET_SIZE,
                                                                   &camera->currentStreamInterfaceAlternate,
                                                                   &camera->unMultipleIsoMaxPacketSize);
                        } else if (USB_SPEED_HIGH == speed) {
                            /* if device is high speed, the selected endpoint's maxPacketSize is maximum device supports
                             * and is not more than 1024B */
                            USB_HostGetProperEndpointInterfaceInfo(camera->classHandle,
                                                                   HIGH_SPEED_ISO_MAX_PACKET_SIZE_ZERO_ADDITION,
                                                                   &camera->currentStreamInterfaceAlternate,
                                                                   &camera->unMultipleIsoMaxPacketSize);
                        }
                        if (camera->currentStreamInterfaceAlternate) {
                            /* set interface by the proper alternate setting */
                            camera->runWaitState = kUSB_HostVideoRunWaitGetSetProbeCommit;
                            camera->runState     = kUSB_HostVideoRunIdle;
                            if (USB_HostVideoStreamSetInterface(
                                    camera->classHandle, camera->streamIntfHandle,
                                    camera->currentStreamInterfaceAlternate, USB_HostVideoControlCallback,
                                    camera) != kStatus_USB_Success) {
                                usb_echo("set interface error\r\n");
                            }
                        } else {
                            /* no proper alternate setting */
                            camera->runWaitState = kUSB_HostVideoRunIdle;
                            camera->runState     = kUSB_HostVideoRunIdle;
                            usb_echo("no proper alternate setting\r\n");
                        }
                    }
                    break;
                default:
                    break;
            }
            break;
        case kUSB_HostVideoRunGetSetProbeCommitDone: {
            OSA_SR_ALLOC();
            if (camera->cameraDeviceFormatType == USB_HOST_DESC_SUBTYPE_VS_FORMAT_MJPEG) {
                /* print the camera device format info*/
                uint32_t w = (uint32_t)(USB_SHORT_FROM_LITTLE_ENDIAN_ADDRESS(camera->videoStreamMjpegFrameDescriptor->wWitd));
                uint32_t h = (uint32_t)(USB_SHORT_FROM_LITTLE_ENDIAN_ADDRESS(camera->videoStreamMjpegFrameDescriptor->wHeight));
                int fps = 10000000 / USB_LONG_FROM_LITTLE_ENDIAN_ADDRESS(s_VideoProbe[idx].dwFrameInterval);
                usb_echo("Camera setting is: %d(w)*%d(h)@%dfps\r\n", w, h, fps);
                usb_echo("picture format is MJPEG\r\n");
            } else if (camera->cameraDeviceFormatType == USB_HOST_DESC_SUBTYPE_VS_FORMAT_UNCOMPRESSED) {
                uint32_t w = (uint32_t)(USB_SHORT_FROM_LITTLE_ENDIAN_ADDRESS(camera->videoStreamUncompressedFrameDescriptor->wWitd));
                uint32_t h = (uint32_t)(USB_SHORT_FROM_LITTLE_ENDIAN_ADDRESS(camera->videoStreamUncompressedFrameDescriptor->wHeight));
                int fps = 10000000 / USB_LONG_FROM_LITTLE_ENDIAN_ADDRESS(s_VideoProbe[idx].dwFrameInterval);
                usb_echo("Camera setting is: %d(w)*%d(h)@%dfps\r\n", w, h, fps);
                usb_echo("picture format is ");
                if (memcmp(&camera->videoStreamUncompressedFormatDescriptor->guidFormat[0],
                           &g_YUY2FormatGUID[0], 16) == 0) {
                    usb_echo("YUY2\r\n");
                } else if (memcmp(&camera->videoStreamUncompressedFormatDescriptor->guidFormat[0],
                                  &g_NV12FormatGUID[0], 16) == 0) {
                    usb_echo("NV12\r\n");
                } else if (memcmp(&camera->videoStreamUncompressedFormatDescriptor->guidFormat[0],
                                  &g_M420FormatGUID[0], 16) == 0) {
                    usb_echo("M420\r\n");
                } else if (memcmp(&camera->videoStreamUncompressedFormatDescriptor->guidFormat[0],
                                  &g_I420FormatGUID[0], 16) == 0) {
                    usb_echo("I420\r\n");
                } else {
                    /* directly print GUID*/
                    for (uint8_t index = 0; index < 16; ++index) {
                        usb_echo("%x ", camera->videoStreamUncompressedFormatDescriptor->guidFormat[index]);
                    }
                    usb_echo("\r\n");
                }
            }

            /** TODO: add the code for discarding invalid format */
            // if (not_supported_format) {
            //     usb_echo(
            //         "picture buffer malloc failed, please make sure the heap size is enough. If it is raw data now, "
            //         "please use other format like MJPEG \r\n");
            //     camera->runState     = kUSB_HostVideoRunIdle;
            //     camera->runWaitState = kUSB_HostVideoRunIdle;
            //     return;
            // }

            /* delay to make sure the device camera is ready, delay 2ms */
            vTaskDelay(pdMS_TO_TICKS(2));
            OSA_ENTER_CRITICAL();
            /* prime multiple transfers */
            for (i = 0; i < USB_VIDEO_STREAM_BUFFER_COUNT; i++) {
                USB_HosVideoStreamRecv(camera->classHandle, (uint8_t *)&s_streamBuffer[idx][i][0],
                                       camera->unMultipleIsoMaxPacketSize, USB_HostVideoStreamDataInCallback, camera);
            }
            OSA_EXIT_CRITICAL();
            camera->runState = kUSB_HostVideoStart;
            usb_echo("USB at video start port %d\r\n", camera->port);
            break;
        }
        case kUSB_HostVideoStart:
            vTaskDelay(pdMS_TO_TICKS(10));
            break;
        default:
            vTaskDelay(pdMS_TO_TICKS(10));
            break;
    }
}

static int send_frames(uint8_t idx) {   
    if (!frames[idx].ready) {
        return 0;
    }

    int buffIdx = frames[idx].pictureBufferIndex;
    int size = check_frame(idx);
    if (size <= 0) {
        frames[idx].ready = 0;
        return 0;
    }

    uint32_t now = get_time();

    // limit to 10 fps
    if (now - frames[idx].last_time < 100) {
        frames[idx].ready = 0;
        return 0;
    }

    frames[idx].last_time = now;

    int currIdx     = 0;
    uint8_t eof     = 0;
    int fragmentNum = 0;
    while (1) {

        int packetSize = MIN(size, PROTOCOL_MAX_MTU);
        size -= packetSize;

        if (size <= 0) {
            eof = 1;
        }

        uint16_t fragment = eof << 15 | frames[idx].frameId << 14 | fragmentNum++;

        if (frames[idx].is_ext) {
            send_ext_video_frame(frames[idx].address[buffIdx], fragment, frames[idx].frame[buffIdx] + currIdx, packetSize);
        } else {
            send_video_frame(frames[idx].slot, fragment, frames[idx].frame[buffIdx] + currIdx, packetSize);
        }
        
        currIdx += packetSize;
        if (eof) {
            break;
        }
    }
    frames[idx].ready = 0;
    return 1;
}

void USB_HostVideoFramesTask(void *param) {
    while (true) {
        int sent = 0;
        for (uint8_t idx = 0; idx < 4; idx++) {
            sent += send_frames(idx);
        }
        if (sent == 0) {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    vTaskDelete(NULL);
}

static usb_host_video_camera_instance_t *USB_Get_Camera(usb_device_handle deviceHandle) {
    uint8_t port = 0;
    (void)USB_HostHelperGetPeripheralInformation(deviceHandle, (uint32_t)kUSB_HostGetDevicePortNumber, (void *)&port);
    usb_host_video_camera_instance_t *camera = port_cameras[port];
    if (camera == NULL && i_Cameras < MAX_USB_CAMERAS) {
        camera = port_cameras[port] = &g_Cameras[i_Cameras];
        camera->port                = port;
        camera->stream_id           = camera_stream_id;
        camera->g_index             = i_Cameras;
        i_Cameras++;
    }
    return camera;
}

/* Implementation of the MJPEG format detection function */
static usb_status_t USB_HostVideoCheckForMJPEG(usb_host_interface_t *hostInterface) {
    uint8_t *descriptorBuffer;
    uint32_t descriptorLength;
    uint8_t descriptorType;
    uint8_t descriptorSubType;

    /* Start from interface extension */
    descriptorBuffer = hostInterface->interfaceExtension;
    descriptorLength = hostInterface->interfaceExtensionLength;
    
    /* Search through all descriptors for this interface */
    while (descriptorLength > 2) {
        /* Check if this is a VS_FORMAT_MJPEG descriptor */
        if ((descriptorBuffer[1] == USB_HOST_DESC_CS_INTERFACE) && 
            (descriptorBuffer[2] == USB_HOST_DESC_SUBTYPE_VS_FORMAT_MJPEG)) {
            return kStatus_USB_Success;
        }
        
        /* Move to next descriptor */
        if (descriptorBuffer[0] == 0) {
            return kStatus_USB_Error; /* Invalid descriptor */
        }
        
        descriptorLength -= descriptorBuffer[0];
        descriptorBuffer += descriptorBuffer[0];
    }
    
    return kStatus_USB_Error; /* No MJPEG format found */
}

usb_status_t USB_HostVideoEvent(usb_device_handle deviceHandle,
                                usb_host_configuration_handle configurationHandle,
                                uint32_t eventCode) {
    usb_status_t status = kStatus_USB_Success;
    uint8_t id;
    usb_host_configuration_t *configuration;
    uint8_t interface_index;
    usb_host_interface_t *hostInterface;
    uint32_t info_value = 0U;

    usb_host_video_camera_instance_t *camera = USB_Get_Camera(deviceHandle);
    if (camera == NULL) {
        usb_echo("DEBUG: Max cameras reached: %d\r\n", MAX_USB_CAMERAS);
        return kStatus_USB_NotSupported;
    }

    uint8_t idx = camera->g_index;

    switch (eventCode) {
        case kUSB_HostEventAttach:
            /* judge whether is configurationHandle supported */
            configuration                      = (usb_host_configuration_t *)configurationHandle;
            g_VideoDeviceHandle[idx]           = NULL;
            g_VideoControlInterfaceHandle[idx] = NULL;
            g_VideoStreamInterfaceHandle[idx]  = NULL;

            for (interface_index = 0U; interface_index < configuration->interfaceCount; ++interface_index) {
                hostInterface = &configuration->interfaceList[interface_index];
                id            = hostInterface->interfaceDesc->bInterfaceClass;
                if (id != USB_HOST_VIDEO_CLASS_CODE) {
                    continue;
                }
                id = hostInterface->interfaceDesc->bInterfaceSubClass;
                if (id == USB_HOST_VIDEO_SUBCLASS_CODE_CONTROL) {
                    g_VideoDeviceHandle[idx]           = deviceHandle;
                    g_VideoControlInterfaceHandle[idx] = hostInterface;
                    continue;
                } else if (id == USB_HOST_VIDEO_SUBCLASS_CODE_STREAM) {
                    /* Prefer interfaces that support MJPEG format */
                    status_t status = USB_HostVideoCheckForMJPEG(hostInterface);
                    if (status == kStatus_USB_Success || g_VideoStreamInterfaceHandle[idx] == NULL) {
                        /* Save this as a fallback if no MJPEG interface is found */
                        g_VideoDeviceHandle[idx]          = deviceHandle;
                        g_VideoStreamInterfaceHandle[idx] = hostInterface;
                    }
                    continue;
                } else {
                }
            }
            if (g_VideoDeviceHandle[idx] != NULL) {
                return kStatus_USB_Success;
            }
            status = kStatus_USB_NotSupported;
            break;

        case kUSB_HostEventNotSupported:
            break;

        case kUSB_HostEventEnumerationDone:
            if (camera->devState == kStatus_DEV_Idle) {
                if ((g_VideoDeviceHandle[idx] != NULL) && (g_VideoControlInterfaceHandle[idx] != NULL)) {
                    camera->devState          = kStatus_DEV_Attached;
                    camera->deviceHandle      = g_VideoDeviceHandle[idx];
                    camera->controlIntfHandle = g_VideoControlInterfaceHandle[idx];
                    camera->streamIntfHandle  = g_VideoStreamInterfaceHandle[idx];
                    USB_HostHelperGetPeripheralInformation(deviceHandle, kUSB_HostGetDevicePID, &info_value);
                    usb_echo("video device attached:pid=0x%x", info_value);
                    USB_HostHelperGetPeripheralInformation(deviceHandle, kUSB_HostGetDeviceVID, &info_value);
                    usb_echo("vid=0x%x ", info_value);
                    USB_HostHelperGetPeripheralInformation(deviceHandle, kUSB_HostGetDeviceAddress, &info_value);
                    usb_echo("address=%d\r\n", info_value);
                }
            } else {
                usb_echo("not idle vide instance\r\n");
            }
            break;

        case kUSB_HostEventDetach:
            if (camera->devState != kStatus_DEV_Idle) {
                camera->devState = kStatus_DEV_Detached;
            }
            break;

        default:
            break;
    }
    return status;
}
