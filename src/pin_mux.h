#ifndef _PRESET_H_
#define _PRESET_H_

#ifdef __cplusplus
extern "C" {
#endif

void BOARD_InitDebugPins(void);
void BOARD_InitCommonPins(void);
void BOARD_InitMotorPins(void);
void BOARD_InitEncoderPins(void);
void BOARD_InitEnetPins(void);
void BOARD_InitEnet1GPins(void);
void BOARD_InitAudioPins(void);

#ifdef __cplusplus
}
#endif

#endif /* _PRESET_H_ */