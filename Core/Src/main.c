/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/*
 * proj_tg ->	Projeto onde eu faco a leitura dos arquivos de audio um cartao
 * 			 	SD para um buffer e depois faco a inferencia com base nos dados
 * 			 	lidos.
 */

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
//#define ARM_MATH_CM4
#include "arm_math.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h> //for va_list var arg functions
#include <math.h>
#include "FeatureExtraction.h"
#include "SDCard.h"
#include "decision_tree_model.h"
//#include "extra_trees_model.h"
//#include "gaussian_naive_bayes_model.h"
//#include "random_forest_model.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
	char chunkID[4];
	uint32_t chunkSize;
	char format[4];
	char subchunk1ID[4];
	uint32_t subchunk1Size;
	uint16_t audioFormat;
	uint16_t numChannels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
	char subchunk2ID[4];
	uint32_t subchunk2Size;
} WAVHeader; //

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define AUDIO_FILE_NAME "audio.wav"
#define INPUT_BUFFER_SIZE 4096
#define HALF_INPUT_BUFFER_SIZE (INPUT_BUFFER_SIZE / 2)
#define FFT_BUFFER_SIZE (INPUT_BUFFER_SIZE / 2)
#define SAMPLE_RATE_HZ 44800 //TODO: MUDAR DE ACORDO COM O VALOR REAL MOSTRADO PELO PROJETO
//#define INT16_TO_FLOAT (1.0 / (32768.0f))
//#define FLOAT_TO_INT16 32768.0f
#define INT_TO_FLOAT (1.0 / (4096.0f))
#define FLOAT_TO_INT 4096.0f
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
//Input Audio Stuff - Variaveis usadas na aquisicao do audio utilizando o microfone
int16_t adcBuffer[INPUT_BUFFER_SIZE];
static volatile float32_t inputBuffer[INPUT_BUFFER_SIZE];
static volatile uint8_t bufferReadyFlag;

// FFT related stuff
arm_rfft_fast_instance_f32 fftHandler;
float32_t hanningWindow[FFT_BUFFER_SIZE]; // Buffer para a janela
float32_t fftBufferInput[FFT_BUFFER_SIZE];
float32_t fftBufferOutput[FFT_BUFFER_SIZE];
static volatile uint8_t fftBufferReadyFlag; //let it know when the FFT buffer is full/is ready to be computed
//static float peakVal = 0.0f;
//static uint32_t peakHz = 0;

//Time Domain Features
static TDFeatures tdFeatures = { 0 }; // time domain features
//static float32_t RMS;
//static float32_t MeanVal; 	/* Removed in the article */
//static float32_t MedianVal; 	/* Removed in the article */
//static float32_t VarianceVal;
//static float32_t SigShapeFactor;
//static float32_t SigKurtosisVal;
//static float32_t SigSkewnessVal;
//static float32_t SigImpulseFactor;
//static float32_t SigCrestFactor;
//static float32_t SigMarginFactor;

//Frequency Domain Features
static FDFeatures fdFeatures = { 0 }; // frequency domain features
//static int32_t PeakAmp1; // largest amplitude of the extracted frequencies in the signal
//static int32_t PeakAmp2; // second largest amplitude of the extracted frequencies in the signal
//static int32_t PeakAmp3; // third largest amplitude of the extracted frequencies in the signal
//static int32_t PeakLocs1; 	// frequency value of the largest amplitude
//static int32_t PeakLocs2; 	// frequency value of the second largest amplitude
//static int32_t PeakLocs3; 	// frequency value of the third largest amplitude
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI2_Init(void);
/* USER CODE BEGIN PFP */

//File System
void myprintf(const char *fmt, ...);
FRESULT readWAVHeader(FIL *file, WAVHeader *header);
FRESULT readWAVData(FIL *file, void *buffer, UINT numBytesToRead,
		UINT *numBytesRead);
void printWAVHeader(const WAVHeader *header);
FRESULT readAllWavFile(FIL *file, WAVHeader *header);

//Audio Processing
void createHanningWindow(float32_t *window, int size);
float32_t calculateKurtosis(float32_t *signal, uint32_t length, float32_t mean,
		float32_t stdDev);
float32_t calculateSkewness(float32_t *signal, uint32_t length, float32_t mean,
		float32_t stdDev);
//void extractTimeDomainFeatures(void);
//void extractFrequencyDomainFeatures(void);
//void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc);
//void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc);
int32_t decision_tree_test(void);
int32_t extra_trees_test(void);
int32_t gaussian_naive_bayes_test(void);
int32_t random_forest_test(void);
int32_t run_inference(int32_t (*func)(void));

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void myprintf(const char *fmt, ...) {
	static char buffer[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	int len = strlen(buffer);
	HAL_UART_Transmit(&huart2, (uint8_t*) buffer, len, -1);

}

FRESULT readWAVHeader(FIL *file, WAVHeader *header) {
	FRESULT res;

	// Ler o cabeçalho do arquivo WAV
	UINT bytesRead;
	res = f_read(file, header, sizeof(WAVHeader), &bytesRead);

	if (res != FR_OK || bytesRead != sizeof(WAVHeader)) {
		// Lidar com erro na leitura do cabeçalho
		return res;
	}

	// Verificar se é um arquivo WAV
	if (strncmp(header->chunkID, "RIFF", 4) != 0
			|| strncmp(header->format, "WAVE", 4) != 0) {
		// Lidar com erro - não é um arquivo WAV válido
		return FR_NOT_ENABLED;
	}

	// Outras verificações e manipulações podem ser adicionadas conforme necessário

	return FR_OK;
}

FRESULT readWAVData(FIL *file, void *buffer, UINT numBytesToRead,
		UINT *numBytesRead) {
	// Ler dados do arquivo WAV
	return f_read(file, buffer, numBytesToRead, numBytesRead);
}

FRESULT readAllWavFile(FIL *file, WAVHeader *header) {
	int buffer[1024];
//	for (int i = 0, i<= header.)
	return 0;
}

void printWAVHeader(const WAVHeader *header) {
	myprintf("chunkID: %.4s \r\n", header->chunkID);
	myprintf("chunkSize: %u \r\n", header->chunkSize);
	myprintf("format: %.4s \r\n", header->format);
	myprintf("subchunk1ID: %.4s \r\n", header->subchunk1ID);
	myprintf("subchunk1Size: %u \r\n", header->subchunk1Size);
	myprintf("audioFormat: %u \r\n", header->audioFormat);
	myprintf("numChannels: %u \r\n", header->numChannels);
	myprintf("sampleRate: %u \r\n", header->sampleRate);
	myprintf("byteRate: %u \r\n", header->byteRate);
	myprintf("blockAlign: %u \r\n", header->blockAlign);
	myprintf("bitsPerSample: %u \r\n", header->bitsPerSample);
	myprintf("subchunk2ID: %.4s \r\n", header->subchunk2ID);
	myprintf("subchunk2Size: %u \r\n", header->subchunk2Size);
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_USART2_UART_Init();
	MX_FATFS_Init();
	MX_SPI2_Init();
	/* USER CODE BEGIN 2 */
	myprintf("\r\n~ Projeto TG by Italo ~\r\n\r\n");
	HAL_Delay(3000); //a short delay is important to let the SD card settle
	arm_rfft_fast_init_f32(&fftHandler, FFT_BUFFER_SIZE); //init FFT Handler
	// Criar o vetor da janela Hanning
	arm_fill_f32(1.0f, hanningWindow, FFT_BUFFER_SIZE); // Preenche o buffer com 1.0 inicialmente
	createHanningWindow(hanningWindow, FFT_BUFFER_SIZE); // Aplica o janelamento Hanning no vetor

	//some variables for FatFs
	FATFS FatFs; 	//Fatfs handle
	FIL fil; 		//File handle
	FRESULT fres; //Result after operations
	WAVHeader wavHeader;
	UINT bytesRead;

	bufferReadyFlag = 0;
	fftBufferReadyFlag = 0;

	//Open the file system
	fres = f_mount(&FatFs, "", 1); //1=mount now
	if (fres != FR_OK) {
		myprintf("f_mount error (%i)\r\n", fres);
		while (1)
			;
	}
	//Now let's try to open file "audio.wav"
	fres = f_open(&fil, "audio.wav", FA_READ);
	if (fres != FR_OK) {
		myprintf("f_open error (%i)\r\n", fres);
		// Desmontar o sistema de arquivos
		f_mount(NULL, "", 0);
		while (1)
			;
	}

	// Ler o cabeçalho do arquivo WAV
	fres = readWAVHeader(&fil, &wavHeader);
	if (fres != FR_OK) {
		// Lidar com erros na leitura do cabeçalho
		myprintf("Error opening wav header(%i)\r\n", fres);
		f_close(&fil);
		// Desmontar o sistema de arquivos
		f_mount(NULL, "", 0);
		while (1)
			;
	}

	printWAVHeader(&wavHeader);

	// Buffer para leitura de dados
	// Verificar se é um arquivo WAV válido
	if (strncmp(wavHeader.chunkID, "RIFF", 4) == 0
			&& strncmp(wavHeader.format, "WAVE", 4) == 0) {
		// Agora você pode acessar os campos do cabeçalho WAV
		uint32_t sampleRate = wavHeader.sampleRate;
		uint16_t numChannels = wavHeader.numChannels;
		uint16_t bitsPerSample = wavHeader.bitsPerSample;

		// Resto do código para processar os dados de áudio...

		// Agora, leia os dados do arquivo com base nas informações do cabeçalho
		uint32_t dataSize = wavHeader.subchunk2Size;
		uint32_t j = 0;

		while (dataSize > 0) {
			UINT bytesToRead =
					(dataSize > INPUT_BUFFER_SIZE) ?
							INPUT_BUFFER_SIZE : dataSize;
			f_read(&fil, adcBuffer, bytesToRead, &bytesRead);

			// Converte os arquivos de entrada de int16_t para float32_t
			for (int i = 0; i < INPUT_BUFFER_SIZE; i++) {

				inputBuffer[i] = (float32_t) adcBuffer[i];
				//	inputBuffer[HALF_INPUT_BUFFER_SIZE+i] = (float32_t) adcBuffer[i];
				//		inputBuffer[HALF_INPUT_BUFFER_SIZE+i] = adcBuffer[i] * INT16_TO_FLOAT;
				//		inputBuffer[HALF_INPUT_BUFFER_SIZE+i] = (float32_t) adcBuffer[i] * INT_TO_FLOAT;

			}

			// Aqui você pode processar os dados do buffer conforme necessário
			for (int n = (FFT_BUFFER_SIZE / 4); n <= FFT_BUFFER_SIZE;
					n = n + (FFT_BUFFER_SIZE / 4)) {

				/*
				 * Pre-processing
				 */
				//TODO: Testar possivel otimizacao removendo essa copia inicial e fazendo a multiplicacao direta
				// 	  direta da janela com o input buffer
				//75% overlapping of the data before windowing
				arm_copy_f32(&inputBuffer[n], fftBufferInput, FFT_BUFFER_SIZE);

				//Apply hanning window to fft input buffer
				arm_mult_f32(fftBufferInput, hanningWindow, fftBufferInput,
				FFT_BUFFER_SIZE);

				/*
				 * Extract Time Domain Features
				 */
				//			  uint32_t startTick = SysTick->VAL;
//				extractTimeDomainFeatures();
				//			  uint32_t endTick = SysTick->VAL;
				/*
				 * Extract Frequency Domain Features
				 */
//				extractFrequencyDomainFeatures();
				/*
				 * Run Inference
				 */
//				int32_t result = run_inference(decision_tree_test);
				//myprintf("%d %d %d %d %d %d \r\n", PeakAmp1, PeakAmp2, PeakAmp3, PeakLocs1, PeakLocs2, PeakLocs3);
//				myprintf("Inference result: %d\r\n", result);

			}

			//updates inputBuffer with previous data for overlapping
			arm_copy_f32(&inputBuffer[HALF_INPUT_BUFFER_SIZE], &inputBuffer[0],
			FFT_BUFFER_SIZE);
			bufferReadyFlag = 0;

		}
		dataSize -= bytesRead;
	}

	else {
		// Não é um arquivo WAV válido
		// Adote a lógica de tratamento de erro apropriada
	}

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */

	f_close(&fil);
// Desmontar o sistema de arquivos
	f_mount(NULL, "", 0);
	while (1) {
		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
		HAL_Delay(1000);
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 16;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
	RCC_OscInitStruct.PLL.PLLQ = 2;
	RCC_OscInitStruct.PLL.PLLR = 2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief SPI2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI2_Init(void) {

	/* USER CODE BEGIN SPI2_Init 0 */

	/* USER CODE END SPI2_Init 0 */

	/* USER CODE BEGIN SPI2_Init 1 */

	/* USER CODE END SPI2_Init 1 */
	/* SPI2 parameter configuration*/
	hspi2.Instance = SPI2;
	hspi2.Init.Mode = SPI_MODE_MASTER;
	hspi2.Init.Direction = SPI_DIRECTION_2LINES;
	hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi2.Init.NSS = SPI_NSS_SOFT;
	hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
	hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi2.Init.CRCPolynomial = 10;
	if (HAL_SPI_Init(&hspi2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN SPI2_Init 2 */

	/* USER CODE END SPI2_Init 2 */

}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void) {

	/* USER CODE BEGIN USART2_Init 0 */

	/* USER CODE END USART2_Init 0 */

	/* USER CODE BEGIN USART2_Init 1 */

	/* USER CODE END USART2_Init 1 */
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */

	/* USER CODE END USART2_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */
	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : B1_Pin */
	GPIO_InitStruct.Pin = B1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : LD2_Pin */
	GPIO_InitStruct.Pin = LD2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : SD_CS_Pin */
	GPIO_InitStruct.Pin = SD_CS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(SD_CS_GPIO_Port, &GPIO_InitStruct);

	/* USER CODE BEGIN MX_GPIO_Init_2 */
	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void createHanningWindow(float32_t *window, int size) {
	for (int n = 0; n < size; n++) {
		window[n] = window[n] * (0.5 - 0.5 * cos(2.0 * M_PI * n / (size - 1)));
	}
}

//float32_t calculateKurtosis(float32_t *signal, uint32_t length, float32_t mean,
//		float32_t stdDev) {
//
//// Inicializa a kurtosis
//	float32_t kurtosis = 0.0f;
//
//// Calcula a kurtosis usando a fórmula
//	for (uint32_t i = 0; i < length; i++) {
//		float32_t diff = signal[i] - mean;
//		kurtosis += (diff * diff * diff * diff)
//				/ (length * stdDev * stdDev * stdDev * stdDev);
//	}
//
//	return kurtosis;
//}

//float32_t calculateSkewness(float32_t *signal, uint32_t length, float32_t mean,
//		float32_t stdDev) {
//
//// Inicializa o skewness
//	float32_t skewness = 0.0f;
//
//// Calcula o skewness usando a fórmula
//	for (uint32_t i = 0; i < length; i++) {
//		float32_t diff = signal[i] - mean;
//		skewness += (diff * diff * diff) / (length * stdDev * stdDev * stdDev);
//	}
//
//	return skewness;
//}

//void extractTimeDomainFeatures(void) {
//
//	float32_t AbsSig[FFT_BUFFER_SIZE];
//	float32_t MaxValue, MinValue, MeanVal, MeanAbs, StdDevValue;
//	uint32_t MaxValueIndex, MinValueIndex;
//
//	/* Calculate Max Value*/
//	arm_max_f32(fftBufferInput, FFT_BUFFER_SIZE, &MaxValue, &MaxValueIndex);
//
//	/* Calculate Min Value*/
//	arm_min_f32(fftBufferInput, FFT_BUFFER_SIZE, &MinValue, &MinValueIndex);
//
//	/* Calculate Absolute Value*/
//	arm_abs_f32(fftBufferInput, AbsSig, FFT_BUFFER_SIZE);
//
//	/* Calculate Mean of the Absolute Signal */
//	arm_mean_f32(AbsSig, FFT_BUFFER_SIZE, &MeanAbs);
//
//	/* Calculate Mean Value */
//	arm_mean_f32(fftBufferInput, FFT_BUFFER_SIZE, &MeanVal);
//
//	/* Calculate RMS */
//	arm_rms_f32(fftBufferInput, FFT_BUFFER_SIZE, &RMS);
//
//	/* Calculate Variance */
//	arm_var_f32(fftBufferInput, FFT_BUFFER_SIZE, &VarianceVal);
//
//	/* Calculate Standart Deviation Value */
//	arm_std_f32(fftBufferInput, FFT_BUFFER_SIZE, &StdDevValue);
//
//	/* Calculate SigShape Value */
//	SigShapeFactor = RMS / MeanAbs;
//
//	/* Calculate Kurtosis */
//	SigKurtosisVal = calculateKurtosis(fftBufferInput, FFT_BUFFER_SIZE, MeanVal,
//			StdDevValue);
//
//	/* Calculate Skewness */
//	SigSkewnessVal = calculateSkewness(fftBufferInput, FFT_BUFFER_SIZE, MeanVal,
//			StdDevValue);
//
//	/* Calculate Impulse Factor */
//	SigImpulseFactor = MaxValue / MeanAbs;
//
//	/* Calculate Crest Factor */
//	SigCrestFactor = MaxValue / RMS;
//
//	/* Calculate Margin Factor */
//	SigMarginFactor = (MaxValue - MinValue) / (MeanVal);
//}

//void extractFrequencyDomainFeatures(void) {
//
//	PeakAmp1 = 0;
//	PeakAmp2 = 0;
//	PeakAmp3 = 0;
//	PeakLocs1 = 0;
//	PeakLocs2 = 0;
//	PeakLocs3 = 0;
//
//	arm_rfft_fast_f32(&fftHandler, fftBufferInput, fftBufferOutput, 0); //the last arg means that we dont want to calc the invert fft
//
////Compute absolute value of complex FFT results per frequency bin, get peak
//	int32_t freqIndex = 4;
////TODO: Verificar novamente se amplitude da posicao 2 continua alta
//	for (int index = 4; index < FFT_BUFFER_SIZE; index += 2) {
//		float curVal = sqrtf(
//				(fftBufferOutput[index] * fftBufferOutput[index])
//						+ (fftBufferOutput[index + 1]
//								* fftBufferOutput[index + 1]));
//
//		if (curVal > PeakAmp1) {
//			PeakAmp3 = PeakAmp2;
//			PeakAmp2 = PeakAmp1;
//			PeakAmp1 = curVal;
//			PeakLocs3 = PeakLocs2;
//			PeakLocs2 = PeakLocs1;
//			PeakLocs1 = (uint32_t) ((freqIndex / 2)
//					* (SAMPLE_RATE_HZ / ((float) FFT_BUFFER_SIZE)));
//		} else if (curVal > PeakAmp2) {
//			PeakAmp3 = PeakAmp2;
//			PeakAmp2 = curVal;
//			PeakLocs3 = PeakLocs2;
//			PeakLocs2 = (uint32_t) ((freqIndex / 2)
//					* (SAMPLE_RATE_HZ / ((float) FFT_BUFFER_SIZE)));
//		} else if (curVal > PeakAmp3) {
//			PeakAmp3 = curVal;
//			PeakLocs3 = (uint32_t) ((freqIndex / 2)
//					* (SAMPLE_RATE_HZ / ((float) FFT_BUFFER_SIZE)));
//		}
//
//		freqIndex = freqIndex + 2;
//	}
//}

void ProcessData() {
}

//void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) {
//
////	arm_copy_f32(&adcBuffer[0], &inputBuffer[HALF_INPUT_BUFFER_SIZE], FFT_BUFFER_SIZE);
//	for (int i = 0; i < FFT_BUFFER_SIZE; i++) {
//		inputBuffer[HALF_INPUT_BUFFER_SIZE + i] = (float32_t) adcBuffer[i];
////		inputBuffer[HALF_INPUT_BUFFER_SIZE+i] = adcBuffer[i] * INT16_TO_FLOAT;
////		inputBuffer[HALF_INPUT_BUFFER_SIZE+i] = (float32_t) adcBuffer[i] * INT_TO_FLOAT;
//
//	}
////habilita flag indica que o buffer esta pronto para processamento
//	bufferReadyFlag = 1;
//
//	HAL_GPIO_TogglePin(D7_GPIO_Port, D7_Pin);
////	HAL_UART_Transmit(&huart2, (uint8_t*)"HALF\r\n", 6, HAL_MAX_DELAY);
//}
//
//void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
//
////	arm_copy_f32(&adcBuffer[HALF_INPUT_BUFFER_SIZE], &inputBuffer[HALF_INPUT_BUFFER_SIZE], FFT_BUFFER_SIZE);
//	for (int i = 0; i < FFT_BUFFER_SIZE; i++) {
//		inputBuffer[HALF_INPUT_BUFFER_SIZE + i] =
//				(float32_t) adcBuffer[HALF_INPUT_BUFFER_SIZE + i];
////		inputBuffer[HALF_INPUT_BUFFER_SIZE+i] = adcBuffer[HALF_INPUT_BUFFER_SIZE+i] * INT16_TO_FLOAT;
////		inputBuffer[HALF_INPUT_BUFFER_SIZE+i] = (float32_t) adcBuffer[HALF_INPUT_BUFFER_SIZE+i] * INT_TO_FLOAT;
//	}
////habilita flag indica que o buffer esta pronto para processamento
//	bufferReadyFlag = 1;
//
//	HAL_GPIO_TogglePin(D7_GPIO_Port, D7_Pin);
////	HAL_UART_Transmit(&huart2, (uint8_t*)"FULL\r\n", 6, HAL_MAX_DELAY);
//}

int32_t decision_tree_test(void) {
	const int n_features = 14;
//	const int n_testcases = testset_samples;
	float32_t features[] = { tdFeatures.RMS, tdFeatures.VarianceVal, tdFeatures.SigShapeFactor, tdFeatures.SigKurtosisVal,
			tdFeatures.SigSkewnessVal, tdFeatures.SigImpulseFactor, tdFeatures.SigCrestFactor, tdFeatures.SigMarginFactor,
			fdFeatures.PeakAmp1, fdFeatures.PeakAmp2, fdFeatures.PeakAmp3, fdFeatures.PeakLocs1, fdFeatures.PeakLocs2, fdFeatures.PeakLocs3 };
	int errors = 0;
//	char msg[80];
	const int32_t out = model_predict(features, n_features);

//	if (out != expect_result) {
//		printf("test-fail sample=%d expect=%d got=%d \r\n", i, expect_result, out);
//			errors += 1;
//	}

//	sprintf(msg, "test decision_tree result=%d \r\n", out);
//	HAL_UART_Transmit(&huart2, (uint8_t*) msg, strlen(msg), HAL_MAX_DELAY);
//	HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", strlen("\r\n"),
//	HAL_MAX_DELAY);

	return out;
}

int32_t extra_trees_test(void) {
	const int n_features = 14;
//	const int n_testcases = testset_samples;
	float32_t features[] = { tdFeatures.RMS, tdFeatures.VarianceVal, tdFeatures.SigShapeFactor, tdFeatures.SigKurtosisVal,
				tdFeatures.SigSkewnessVal, tdFeatures.SigImpulseFactor, tdFeatures.SigCrestFactor, tdFeatures.SigMarginFactor,
				fdFeatures.PeakAmp1, fdFeatures.PeakAmp2, fdFeatures.PeakAmp3, fdFeatures.PeakLocs1, fdFeatures.PeakLocs2, fdFeatures.PeakLocs3 };

	int errors = 0;
	char msg[80];
	const int32_t out = model_predict(features, n_features);

//	if (out != expect_result) {
//		printf("test-fail sample=%d expect=%d got=%d \r\n", i, expect_result, out);
//			errors += 1;
//	}

	sprintf(msg, "test extra_trees result=%d \r\n", out);
	HAL_UART_Transmit(&huart2, (uint8_t*) msg, strlen(msg), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", strlen("\r\n"),
	HAL_MAX_DELAY);

	return out;
}

int32_t gaussian_naive_bayes_test(void) {
	const int n_features = 14;
//	const int n_testcases = testset_samples;
	float32_t features[] = { tdFeatures.RMS, tdFeatures.VarianceVal, tdFeatures.SigShapeFactor, tdFeatures.SigKurtosisVal,
				tdFeatures.SigSkewnessVal, tdFeatures.SigImpulseFactor, tdFeatures.SigCrestFactor, tdFeatures.SigMarginFactor,
				fdFeatures.PeakAmp1, fdFeatures.PeakAmp2, fdFeatures.PeakAmp3, fdFeatures.PeakLocs1, fdFeatures.PeakLocs2, fdFeatures.PeakLocs3 };

	int errors = 0;
	char msg[80];
	const int32_t out = model_predict(features, n_features);

//	if (out != expect_result) {
//		printf("test-fail sample=%d expect=%d got=%d \r\n", i, expect_result, out);
//			errors += 1;
//	}

	sprintf(msg, "test gaussian_naive_bayes result=%d \r\n", out);
	HAL_UART_Transmit(&huart2, (uint8_t*) msg, strlen(msg), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", strlen("\r\n"),
	HAL_MAX_DELAY);

	return out;
}

int32_t random_forest_test(void) {
	const int n_features = 14;
//	const int n_testcases = testset_samples;
	float32_t features[] = { tdFeatures.RMS, tdFeatures.VarianceVal, tdFeatures.SigShapeFactor, tdFeatures.SigKurtosisVal,
				tdFeatures.SigSkewnessVal, tdFeatures.SigImpulseFactor, tdFeatures.SigCrestFactor, tdFeatures.SigMarginFactor,
				fdFeatures.PeakAmp1, fdFeatures.PeakAmp2, fdFeatures.PeakAmp3, fdFeatures.PeakLocs1, fdFeatures.PeakLocs2, fdFeatures.PeakLocs3 };

	int errors = 0;
	char msg[80];
	const int32_t out = model_predict(features, n_features);

//	if (out != expect_result) {
//		printf("test-fail sample=%d expect=%d got=%d \r\n", i, expect_result, out);
//		errors += 1;
//	}

	sprintf(msg, "test random_forest result=%d \r\n", out);
	HAL_UART_Transmit(&huart2, (uint8_t*) msg, strlen(msg), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", strlen("\r\n"),
	HAL_MAX_DELAY);

	return out;
}

int32_t run_inference(int32_t (*func)(void)) {

	return func();
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
