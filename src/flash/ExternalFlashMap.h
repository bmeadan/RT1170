#ifndef EXTERNALFLASHMAP_H_
#define EXTERNALFLASHMAP_H_
#include <stdint.h>

/*
   This file defines the external FLASH mapping. External FLASH

   This chip has 128M bit storage divided into:
   Page size is 256 bytes
   Sector size is 16 pages, hence 4K Byte
   Small Block size is 8 sectors, hence 32K Byte
   Large Block size is 16 sectors, hence 64K Byte

   This chip has more than 100,000 write/erase cycles

   The flash is QSPI that is mapped at address 0x30000000

   *** the external flash is defined as 0x4000000 in size so only 8Mbyte is used... ***

   Minimum erase size is a sector that equals to 4K bytes.
*/

/*
   FLASH chip is IS25WP128

   We use a FLASH with a total storage of 128MBit (16MB) but use only half.
   So flash size is defined in the linker as space = 0x4000000
   Flash memory is divided as follows:

   flash base address is 0x30000000

0x30000000  /------------------------------------------/
            /  Boot Loader storage 64K                 /
0x3000FFFF  /------------------------------------------/
0x30010000  /------------------------------------------/
            /  Boot Config area 4K (+ 60K reserved)    /
0x3001FFFF  /------------------------------------------/
0x30020000  /------------------------------------------/
            /  App Config area 4K  (+ 60K reserved)    /
0x3002FFFF  /------------------------------------------/
0x30030000  /------------------------------------------/
            /  Reserved                                /
0x301FFFFF  /------------------------------------------/
0x30200000  /------------------------------------------/
            /  FW Image A 2MB                          /
0x303FFFFF  /------------------------------------------/
0x30400000  /------------------------------------------/
            /  FW Image B 2MB                          /
0x305FFFFF  /------------------------------------------/
0x30600000  /------------------------------------------/
            /  FW Image G 2MB                          /
0x307FFFFF  /------------------------------------------/
0x30800000  /------------------------------------------/
            /  Reserved                                /
0x3FFFFFFF  /------------------------------------------/

*/

#define IMAGE_SIZE    0x2000000U
#define BOOT_CONFIG_S 0x30010000U
#define APP_CONFIG_S  0x30020000U
#define IMAGE_A_S     0x30200000U
#define IMAGE_B_S     0x30400000U
#define IMAGE_G_S     0x30600000U

//----------------------------------------------------------
// the config sector will hold a structure with the following data.
// index for the image to load, checksum for each image.
// checksum will be a 32 bit !checksum

typedef enum ImagesOptions : uint32_t
{
  Image_A = 0, // Default image
  Image_B = 1, // Update image
  Image_G = 2, // Golden image
  NumberOfImages = 3
} ImagesOptions_t;

typedef struct __attribute__((packed)) ImageData
{
  uint32_t ImageSize;
  uint32_t ImageChecksum;
} ImageData_t;

typedef struct __attribute__((packed)) BootConfigData
{
  uint32_t ImageIdx; // NumberOfImages_t
  ImageData_t ImageData[NumberOfImages];
} BootConfigData_t;


#define CALIBRATION_SIZE 4

typedef struct __attribute__((packed)) AppConfigData
{
  uint16_t calibration[CALIBRATION_SIZE];
  uint8_t flipped_drive: 1; // Reverse drive enabled
  uint32_t chksum;
} AppConfigData_t;

uint32_t GetImageBaseAddress(int ImageIdx)
{
  switch (ImageIdx)
  {
  case Image_A:
    return IMAGE_A_S;
  case Image_B:
    return IMAGE_B_S;
  case Image_G:
    return IMAGE_G_S;
  }
  return 0;
}

uint32_t CalculateApplicationChecksum(uint8_t *ImageAddress, uint32_t ImageSize)
{
  uint32_t Checksum = 0;

  if (ImageSize > IMAGE_SIZE)
  {
    return 0;
  }

  for (int i = 0; i < ImageSize; i++)
  {
    Checksum += ImageAddress[i];
  }

  return Checksum;
}

#endif /* EXTERNALFLASHMAP_H_ */
