/*
 * SDCard.c
 *
 *  Created on: May 21, 2024
 *      Author: Italo LANZA
 */

#include "SDCard.h"
#include "fatfs.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>

static FATFS *fs;	// Sistema de arquivos
static FIL file;   	// Arquivo atual
static int isMounted = 0;
static int isFileOpen = 0;

void SDCard_Init(FATFS *handle) {
	fs = handle;
}

FRESULT SDCard_Mount(void) {
	FRESULT res = f_mount(fs, "", 1); //1=mount now
	if (res == FR_OK) {
		isMounted = 1;
	}
	return res;
}

FRESULT SDCard_Unmount(void) {
	FRESULT res;
	if (isMounted) {
		res = f_mount(NULL, "", 1);
		isMounted = 0;
	}
	return res;
}

FRESULT SDCard_OpenFile(FIL *fileHandle, const char *path, BYTE mode) {
	return f_open(fileHandle, path, mode);
}

FRESULT SDCard_CloseFile(FIL *fileHandle) {
	return f_close(fileHandle);
}

FRESULT SDCard_Read(FIL *fileHandle, void *buffer, size_t bytesToRead,
		size_t *bytesRead) {
	if (isFileOpen) {
		UINT br;
		FRESULT res = f_read(fileHandle, buffer, bytesToRead, &br);
		*bytesRead = br;
		return res;
	}
	return FR_NO_FILE;
}

FRESULT SDCard_Write(FIL *fileHandle, const void *buffer, size_t bytesToWrite,
		size_t *bytesWritten) {
	if (isFileOpen) {
		UINT bw;
		FRESULT res = f_write(fileHandle, buffer, bytesToWrite, &bw);
		*bytesWritten = bw;
		return res;
	}
	return FR_NO_FILE;
}

FRESULT SDCard_ReadLine(FIL *fileHandle, char *buffer, size_t bufferSize) {
	if (!isFileOpen) {
		return FR_NO_FILE;
	}

	size_t index = 0;
	char ch;
	UINT bytesRead;

	while (index < bufferSize - 1) {
		FRESULT res = f_read(fileHandle, &ch, 1, &bytesRead);
		if (res != FR_OK || bytesRead == 0) {
			if (index == 0) {
				return FR_DISK_ERR; // Falha ao ler ou EOF no início
			}
			break; // EOF alcançado
		}

		if (ch == '\n') {
			break; // Linha completa
		}

		buffer[index++] = ch; //Salva char e incrementa o index
	}

	buffer[index] = '\0'; // Terminar a string

	return FR_OK;
}

FRESULT SDCard_WriteLine(FIL *fileHandle, const char *line) {
	if (isFileOpen) {
		size_t len = strlen(line);
		size_t bytesWritten;
		if (SDCard_Write(fileHandle, line, len, &bytesWritten) != FR_OK
				|| bytesWritten != len) {
			return FR_DISK_ERR;
		}

		const char newline[] = "\n";
		if (SDCard_Write(fileHandle, newline, sizeof(newline) - 1,
				&bytesWritten) != FR_OK
				|| bytesWritten != (sizeof(newline) - 1)) {
			return FR_DISK_ERR;
		}

		return FR_OK;
	}
	return FR_NO_FILE;
}

FRESULT SDCard_OpenFile_S(const char *path, BYTE mode) {
	if (isMounted) {
		FRESULT res = SDCard_OpenFile(&file, path, mode);
		if (res == FR_OK) {
			isFileOpen = 1;
		}
		return res;
	}
	return FR_NO_FILESYSTEM;
}

FRESULT SDCard_CloseFile_S(void) {
	if (isFileOpen) {
		isFileOpen = 0;
		return SDCard_CloseFile(&file);;
	}
	return FR_NO_FILE;
}

FRESULT SDCard_Read_S(void *buffer, size_t bytesToRead, size_t *bytesRead) {
	return SDCard_Read(&file, buffer, bytesToRead, bytesRead);
}

FRESULT SDCard_Write_S(const void *buffer, size_t bytesToWrite,
		size_t *bytesWritten) {
	return SDCard_Write(&file, buffer, bytesToWrite, bytesWritten);
}

FRESULT SDCard_ReadLine_S(char *buffer, size_t bufferSize) {
	return SDCard_ReadLine(&file, buffer, bufferSize);
}

FRESULT SDCard_WriteLine_S(const char *buffer) {
	return SDCard_WriteLine(&file, buffer);
}

FRESULT SDCard_DeleteFile_S(const char *path) {
	if (isMounted) {
		return f_unlink(path);
	}
	return FR_NO_FILESYSTEM;
}

