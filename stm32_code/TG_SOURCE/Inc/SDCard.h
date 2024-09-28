/*
 * SDCard.h
 *
 *  Created on: May 21, 2024
 *      Author: Italo LANZA
 * Description: Cabecalho das funcoes relacionados a leitura e escrita
 * 				de arquivos no SD card.
 */

#ifndef INC_SDCARD_H_
#define INC_SDCARD_H_

/* Includes ------------------------------------------------------------------*/
#include "fatfs.h"
#include <string.h>
#include <stdio.h>
/* Typedef -----------------------------------------------------------*/
/* Functions prototypes ---------------------------------------------*/
void SDCard_Init(FATFS *handle);
FRESULT SDCard_Mount(void);
FRESULT SDCard_Unmount(void);
FRESULT SDCard_OpenFile(FIL *fileHandle, const char *path, BYTE mode);
FRESULT SDCard_CloseFile(FIL *fileHandle);
FRESULT SDCard_Read(FIL *fileHandle, void *buffer, size_t bytesToRead,
		size_t *bytesRead);
FRESULT SDCard_Write(FIL *fileHandle, const void *buffer, size_t bytesToWrite,
		size_t *bytesWritten);
FRESULT SDCard_ReadLine(FIL *fileHandle, char *buffer, size_t bufferSize);
FRESULT SDCard_WriteLine(FIL *fileHandle, const char *line);
FRESULT SDCard_OpenFile_S(const char *path, BYTE mode);
FRESULT SDCard_CloseFile_S(void);
FRESULT SDCard_Read_S(void *buffer, size_t bytesToRead, size_t *bytesRead);
FRESULT SDCard_Write_S(const void *buffer, size_t bytesToWrite,
		size_t *bytesWritten);
FRESULT SDCard_ReadLine_S(char *buffer, size_t bufferSize);
FRESULT SDCard_WriteLine_S(const char *buffer);
FRESULT SDCard_DeleteFile_S(const char *path);

#endif /* INC_SDCARD_H_ */
