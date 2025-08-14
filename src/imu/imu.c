#include "imu.h"
#include "imu_lpuart.h"
#include "clock_config.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

static imu_data_t imuData;
static imu_data_offset_t imuDataOffset = {0, 0, 0}; // Initialize offsets to zero
static volatile bool imuCalibrate = false;

static int16_t unwrap_angle(int16_t angle, int16_t offset) {
    int32_t result = angle - offset;
    result = ((result + 18000) % 36000 + 36000) % 36000 - 18000;
    return (int16_t)result;
}

imu_data_t get_imu_data(void)
{
    return imuData;
}
/*
 * @brief Read data from the IMU sensor and populate the provided IMUData structure.
 *
 * @param success Reference to a boolean variable, true if the data was successfully read and the checksum is correct.
 * @param imuData Reference to the IMUData structure to populate with the sensor data.
 */
static void IMU_readIMU(bool *success, imu_data_t *imuData)
{
    static uint8_t buffer[MSG_LENGTH];
    static uint8_t bufferIndex = 0;
    static bool headerReceived = false; // Becomes true once the 2-byte header is detected.
    uint16_t receivedHeader;    // Stores 2-byte header value
    uint8_t byteRcvd;

    *success = false;

    // Read from the LPUART buffer into the IMU data buffer
    while (UART_ReadFromRingBuffer(&byteRcvd, 1))
    {
        if (!headerReceived)
        {
            // Collect bytes to form the header
            buffer[bufferIndex++] = byteRcvd;
            if (bufferIndex == 2)
            {
                receivedHeader = (buffer[0] << 8) | buffer[1];
                if (receivedHeader == HEADER)
                    headerReceived = true;
                else
                    bufferIndex = 0;
            }
        }
        else
        {
            // Collect remaining bytes of the message
            buffer[bufferIndex++] = byteRcvd;
            if (bufferIndex == MSG_LENGTH)
            {
                // Calculate checksum
                uint8_t checksum = 0;
                for (int i = 2; i < MSG_LENGTH - 1; i++)
                    checksum += buffer[i];

                bool isChecksumCorrect = (checksum == buffer[MSG_LENGTH - 1]);

                if (isChecksumCorrect)
                {
                    // Extract IMU data from the buffer
                    int16_t yaw = (buffer[3]) | (buffer[4] << 8);
                    int16_t pitch = (buffer[5]) | (buffer[6] << 8);
                    int16_t roll = (buffer[7]) | (buffer[8] << 8);
                    
                    if (imuCalibrate) {
                        imuDataOffset.yaw = yaw;
                        imuDataOffset.pitch = pitch;
                        imuDataOffset.roll = roll;
                        imuCalibrate = false; // Reset calibration flag after calibration
                    }
                    // Unwrap angles and apply offsets
                    imuData->yaw = unwrap_angle(yaw, imuDataOffset.yaw);
                    imuData->pitch = unwrap_angle(pitch, imuDataOffset.pitch);
                    imuData->roll = unwrap_angle(roll, imuDataOffset.roll);
                    imuData->acc_x = (buffer[9]) | (buffer[10] << 8);
                    imuData->acc_y = (buffer[11]) | (buffer[12] << 8);
                    imuData->acc_z = (buffer[13]) | (buffer[14] << 8);
                }

                // Reset state for next message
                bufferIndex = 0;
                headerReceived = false;
                *success = isChecksumCorrect;
                return;
            }
        }
    }
}

static void imuTask(void *params)
{
    bool imuSuccess;
    while (1)
    {
        IMU_readIMU(&imuSuccess, &imuData);
        vTaskDelay(pdMS_TO_TICKS(10)); // FreeRTOS delay (10ms)
    }
}

void calibrate_imu(void)
{
    imuCalibrate = true;
}

void init_imu(void)
{
    // Initialize the LPUART for communication
    init_imu_lpuart();

    xTaskCreate(imuTask, "IMU_Task", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
}
