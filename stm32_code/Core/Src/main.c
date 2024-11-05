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
/*
 * health.wav
 * f1.wav
 * f2.wav
 * f3.wav
 * off.wav
 */

#define AUDIO_FILE_NAME "health.wav"
#define CSV_FILE_NAME "health.csv"
#define CSV_HEADER "RMS,Variance,Skewness,Kurtosis,CrestFactor,ShapeFactor,ImpulseFactor,MarginFactor,Peak1,Peak2,Peak3,PeakLocs1,PeakLocs2,PeakLocs3"
#define INPUT_BUFFER_SIZE 4096
#define HALF_INPUT_BUFFER_SIZE (INPUT_BUFFER_SIZE / 2)
#define INPUT_SIGNAL_SIZE (INPUT_BUFFER_SIZE * 2)
#define OUTPUT_SIGNAL_SIZE (INPUT_SIGNAL_SIZE / 2)
#define SAMPLE_RATE_HZ 44800 											//TODO: MUDAR DE ACORDO COM O VALOR REAL MOSTRADO PELO PROJETO
#define OVERLAP_FACTOR 0.75  											// Overlapping de 75%
#define ADVANCE_SIZE (int)(INPUT_BUFFER_SIZE * (1.0f - OVERLAP_FACTOR)) // 25% de avanco

//#define INT16_TO_FLOAT (1.0 / (32768.0f))
//#define FLOAT_TO_INT16 32768.0f
#define INT_TO_FLOAT (1.0 / (4096.0f))
#define FLOAT_TO_INT 4096.0f

// Definir apenas uma dessas opcoes:
#define USE_SD_AUDIO           		// Usar audio do cartao SD
//#define USE_MIC_AUDIO             // Usar microfone (ADC + DMA)

// Verificação para garantir que apenas uma opcao esta ativa
#if defined(USE_SD_AUDIO) && defined(USE_MIC_AUDIO)
	#error "Vocc deve definir apenas uma das opcoes: USE_SD_AUDIO ou USE_MIC_AUDIO"
#endif

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
// Variaveis comuns
static volatile int16_t inputBuffer[INPUT_BUFFER_SIZE] = {0};	// Buffer para armazenar os dados de entrada (ADC ou arquivo .wav)
static float32_t inputSignal[INPUT_SIGNAL_SIZE] = {0}; 		   	// Buffer de entrada da FFT
float32_t outputSignal[OUTPUT_SIGNAL_SIZE] = {0}; 				// Buffer de saída para os resultados da FFT
float32_t hanningWindow[OUTPUT_SIGNAL_SIZE]= {0}; 				// Buffer para a janela de hanning
//static volatile uint8_t fftBufferReadyFlag; 					// Variavel que informa se o buffer entrada da FFT esta pronto para processamento
arm_rfft_fast_instance_f32 fftHandler;							// Esturura para FFT real

#ifdef USE_SD_AUDIO
int16_t volatile audioBuffer[INPUT_BUFFER_SIZE] = {0}; // Buffer para armazenar amostras de áudio lidas do arquivo WAV
FATFS FatFs; 							// Estrutura do FatFs
FIL inputFile;           				// Estrtura de um arquivo do FatFs (Arquivo WAV no cartão SD)
FIL outputFile;           				// Estrtura de um arquivo do FatFs (Arquivo csv no cartão SD)
FRESULT fres; 							// Estutura de resultado de uma operacao do FatFs
UINT bytesRead;                     	// Numero de bytes lidos do arquivo
WAVHeader wavHeader;					// Estura de um cabecalho de um arquivo .wav
uint32_t dataSize;						// Variavel para armazenar a quantidade de dados lida do sd card
#endif

#ifdef USE_MIC_AUDIO
	int16_t adcBuffer[INPUT_BUFFER_SIZE];		// Buffer de dados do ADC para capturar audio do microfone
	static volatile uint8_t bufferReadyFlag;	// Variavel que informa se o inputBuffer esta pronto para leitura
	ADC_HandleTypeDef hadc1;					// Estrutura do ADC
	TIM_HandleTypeDef htim2;					// Estrutura do Timer 2 para o DMA
#endif

// Time Domain Features
static TDFeatures tdFeatures = { 0 }; // Estrutura das features relacionadas ao dominio do Tempo

// Frequency Domain Features
static FDFeatures fdFeatures = { 0 }; // Estrutura das features relacionadas ao dominio da Frequencia

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI2_Init(void);
/* USER CODE BEGIN PFP */

// Prototipos relacionados a leitura de arquivos de audio no SDCard
#ifdef USE_SD_AUDIO
FRESULT readWAVHeader(FIL *file, WAVHeader *header);
FRESULT readWAVData(FIL *file, void *buffer, UINT numBytesToRead,
		UINT *numBytesRead);
void printWAVHeader(const WAVHeader *header);
FRESULT readAllWavFile(FIL *file, WAVHeader *header);
#endif

// Pre-processamento
void createHanningWindow(float32_t *window, int size);

// Funcoes de suporte
void myprintf(const char *fmt, ...);
void printFeatures(TDFeatures *tdFeat, FDFeatures *fdFeat);
void formatFeaturestoString(char **bufferPtr, TDFeatures *tdFeat, FDFeatures *fdFeat);
int createFormatedString(char **bufferPtr, const char *fmt, ...);



// Funcoes relacionado a inferencia
int32_t decision_tree_test(void);
int32_t extra_trees_test(void);
int32_t gaussian_naive_bayes_test(void);
int32_t random_forest_test(void);
int32_t run_inference(int32_t (*func)(void));

#ifdef USE_MIC_AUDIO
	void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc);
	void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc);
	static void ADC_Init(void);
	//static void MX_ADC1_Init(void);
	//static void MX_TIM2_Init(void);
#endif

/* USER CODE END PFP */

/* Private user code -----------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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

	arm_rfft_fast_init_f32(&fftHandler, OUTPUT_SIGNAL_SIZE); // Inicializa Estrutura Handler da FFT

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */
	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_USART2_UART_Init();
	MX_SPI2_Init();
	/* USER CODE BEGIN 2 */

	uint32_t sampleRate;
//	uint16_t numChannels;
//	uint16_t bitsPerSample;

#ifdef USE_SD_AUDIO
	MX_FATFS_Init();
	HAL_Delay(3000); 		// Um delay para o cartao SD estabilizar
	SDCard_Init(&FatFs);

	// Montagem do file system
	fres = SDCard_Mount();
	if (fres != FR_OK) {
		myprintf("[ERRO] Erro no SDCard_Mount. Codigo do erro: (%i)\r\n", fres);
		while (1) {
		} // loop infinito para travar a execucao do programa
	}

	// Tenta abrir o arquivo com o nome defino pela flag AUDIO_FILE_NAME
	fres = SDCard_OpenFile(&inputFile, AUDIO_FILE_NAME, FA_READ);
	if (fres != FR_OK) {
		myprintf("[ERRO] Erro ao abrir arquivo '%s'. Codigo do erro: (%i)\r\n",
				AUDIO_FILE_NAME, fres);

		SDCard_Unmount();

		while (1) {
		} // loop infinito para travar a execucao do programa
	}

	// Tenta criar o arquivo com o nome defino pela flag CSV_FILE_NAME
	fres = SDCard_OpenFile(&outputFile, CSV_FILE_NAME, FA_CREATE_ALWAYS | FA_WRITE);
		if (fres != FR_OK) {
			myprintf("[ERRO] Erro ao abrir arquivo '%s'. Codigo do erro: (%i)\r\n",
					CSV_FILE_NAME, fres);

			SDCard_CloseFile(&inputFile);
			SDCard_Unmount();

			while (1) {
			} // loop infinito para travar a execucao do programa
		}

	// Ler o cabeçalho do arquivo WAV
	fres = readWAVHeader(&inputFile, &wavHeader);
	if (fres != FR_OK) {
		// Lidar com erros na leitura do cabeçalho
		myprintf(
				"[ERRO] Erro ao abrir arquivo '.wav'. Codigo do erro: (%i)\r\n",
				fres);

		SDCard_CloseFile(&inputFile);
		SDCard_CloseFile(&outputFile);
		SDCard_Unmount();

		while (1) {
		} // loop infinito para travar a execucao do programa
	}

	printWAVHeader(&wavHeader);

	// Verificar se é um arquivo WAV válido
	if (strncmp(wavHeader.chunkID, "RIFF", 4) == 0
			&& strncmp(wavHeader.format, "WAVE", 4) == 0) {

		// Acessando os campos do cabeçalho WAV
		sampleRate = wavHeader.sampleRate;
//		numChannels = wavHeader.numChannels;
//		bitsPerSample = wavHeader.bitsPerSample;

		// Agora, leia os dados do arquivo com base nas informações do cabeçalho
		dataSize = wavHeader.subchunk2Size;

		HAL_Delay(1000); 		// Um delay para o cartao SD estabilizar
	} else {
		// Não é um arquivo WAV válido
		// TODO: Adotar lógica de tratamento de erro apropriada
		myprintf("[ERRO] Arquivo de audio '.wav' nao e valido!");
		while (1) {
		} // loop infinito para travar a execucao do programa
	}

#endif

#ifdef USE_MIC_AUDIO
	bufferReadyFlag = 0;

	void ADC_Init(void);

	// Habilida o timer 2
	if(HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1) != HAL_OK) {
		Error_Handler();
	}
#endif

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */

	myprintf("\r\n~ Projeto TG by Italo ~\r\n\r\n");

	// Criar o vetor da janela Hanning
//	arm_fill_f32(0.0f, inputSignal, OUTPUT_SIGNAL_SIZE); // Preenche o inputSignal com 0.0 inicialmente
	arm_fill_f32(1.0f, hanningWindow, OUTPUT_SIGNAL_SIZE); // Preenche o hanningWindow com 1.0 inicialmente
	createHanningWindow(hanningWindow, OUTPUT_SIGNAL_SIZE); // Aplica o janelamento Hanning no vetor

	myprintf("\r\n~ Processando dados ~\r\n\r\n");

	char *outputString; 	// variavel para armazenar a string de saida
//	int outputStringSize;	// tamanho da string descontando o caracter nulo

	SDCard_WriteLine(&outputFile, CSV_HEADER);

	while (1) {
#ifdef USE_SD_AUDIO

		size_t bytesToRead =
				(dataSize > INPUT_BUFFER_SIZE) ? INPUT_BUFFER_SIZE : dataSize;
//		myprintf("\r\n~ Lendo arquivo do cartao SD ~\r\n\r\n");

		fres = SDCard_Read(&inputFile, inputBuffer, bytesToRead, &bytesRead);

//		myprintf("[INFO] SDCard_Read. fres= %d , bytesToRead= %d, bytesRead= %d", fres, bytesToRead, bytesRead);
		// Ler amostras de audio do arquivo .wav no cartão SD
		if ( fres == FR_OK && bytesRead >= INPUT_BUFFER_SIZE) {
			// Converter as amostras de 16 bits para float32_t e normalizar
			for (int i = 0; i < INPUT_BUFFER_SIZE; i++) {
				// Copia os dados para segunda metade do inputSignal
				inputSignal[INPUT_BUFFER_SIZE + i] = (float32_t) inputBuffer[i];
//				inputSignal[INPUT_BUFFER_SIZE + i] = (float32_t) inputBuffer[i] / 32768.0f; // Normalizacao de 16 bits para float
			}
		} else {
			// Erro de leitura ou fim de arquivo
			//f_lseek(&inputFile, 44); 	// Reinicia o arquivo (ciclo)

			// Fecha os arquivos WAV, CSV e desmontar o sistema de arquivos
			SDCard_CloseFile(&inputFile);
			myprintf("[INFO] Fechando arquivo \"%s\". Codigo do erro: (%i)\r\n", AUDIO_FILE_NAME, fres);
			SDCard_CloseFile(&outputFile);
			myprintf("[INFO] Fechando arquivo \"%s\". Codigo do erro: (%i)\r\n", CSV_FILE_NAME, fres);
			SDCard_Unmount();
			myprintf("[INFO] Desmontando FatFs. Codigo do erro: (%i)\r\n", fres);
			myprintf("\r\n~ Fim do processamento ~\r\n\r\n");
			break;
		}
#endif

#ifdef USE_MIC_AUDIO
		// Capturar dados do microfone via ADC
		sampleRate = SAMPLE_RATE_HZ;


		// Copia os dados para segunda metade do inputSignal
		for (int i = 0; i < INPUT_BUFFER_SIZE; i++) {
			inputSignal[INPUT_BUFFER_SIZE + i] = (float32_t) inputBuffer[i];
//			inputSignal[INPUT_BUFFER_SIZE + i] = (float32_t) adcBuffer[i] / 4096.0f; // Normalizaçcao de 12 bits (4096) para float
		}
#endif
		/* Pre-processamento --------------------------------------------------------*/

		// TODO: Testar possivel otimizacao removendo essa copia inicial e
		//       fazendo a multiplicacao direta da janela com o input buffer
		//  Overlapping de 75% antes do janelamento
//			arm_copy_f32(&inputBuffer[n], inputSignal, OUTPUT_SIGNAL_SIZE);
		// Aplica a janela de hanning no buffer de entrada da fft
		arm_mult_f32(inputSignal, hanningWindow, inputSignal,
				OUTPUT_SIGNAL_SIZE);
		float32_t data[INPUT_BUFFER_SIZE];
		// Processamento dos dados lidos (no buffer) necessário
		//  Overlapping de 75% antes do janelamento
		for (int n = (int) (INPUT_BUFFER_SIZE * (1.0f - OVERLAP_FACTOR));
				n <= INPUT_BUFFER_SIZE;
				n = n + ADVANCE_SIZE ) {

			//TODO: Implementar buffer intermediario para uso na fft\
			// Copia dados do input buffer para array que vai ser utilizado para processamento
			arm_copy_f32(&inputSignal[n], &data[0], OUTPUT_SIGNAL_SIZE);
			/* Extracao das Features no Dominio do Tempo --------------------------------*/

//			uint32_t startTick = SysTick->VAL;
			extractTimeDomainFeatures(&tdFeatures, &data,
					INPUT_BUFFER_SIZE);
//			uint32_t endTick = SysTick->VAL;

			/* Extracao das Features no Dominio da Frequencia ---------------------------*/

			// Calcula fft usando a biblioteca da ARM
			arm_rfft_fast_f32(&fftHandler, &data, outputSignal, 0); // o ultimo argumento significa que nao queremos calcular a fft inversa

//			uint32_t startTick = SysTick->VAL;
			extractFrequencyDomainFeatures(&fdFeatures, outputSignal,
					OUTPUT_SIGNAL_SIZE, sampleRate);
//			uint32_t endTick = SysTick->VAL;

			/* Faz Inferencia -----------------------------------------------------------*/
//			int32_t result = run_inference(decision_tree_test);
			// TODO: Imprimir no console as features
			printFeatures(&tdFeatures, &fdFeatures);
			formatFeaturestoString(&outputString, &tdFeatures, &fdFeatures);
			fres =  SDCard_WriteLine(&outputFile, outputString);
			if (fres != FR_OK) {
				myprintf("[ERRO] Erro ao escrever linha no arquivo '%s'. Codigo do erro: (%i)\r\n", CSV_FILE_NAME, fres);
			}

//			myprintf("Inference result: %d\r\n", result);
			free(outputString); // libera a memoria alocada na funcao de formatar string
		}

		// Atualiza a primeira metade do inputSignal para proxima janela de dados
		arm_copy_f32(&inputSignal[INPUT_BUFFER_SIZE], &inputSignal[0],
				OUTPUT_SIGNAL_SIZE);

		// Desaloca memoria usado para escrever no SDCard
#ifdef USE_MIC_AUDIO
		bufferReadyFlag = 0;
#endif
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}

	myprintf("\r\n~ Fim ~\r\n\r\n");
	while (1) {
		HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
		HAL_Delay(1000);
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

void myprintf(const char *fmt, ...) {
	static char buffer[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	int len = strlen(buffer);
	HAL_UART_Transmit(&huart2, (uint8_t*) buffer, len, -1);

}

void printFeatures(TDFeatures *tdFeat, FDFeatures *fdFeat) {
//	RMS,~~Mean~~,~~Median~~,Variance,Skewness,Kurtosis,CrestFactor,ShapeFactor,ImpulseFactor,MarginFactor,Peak1,Peak2,Peak3,PeakLocs1,PeakLocs2,PeakLocs3,~~FalutID~~
	myprintf("%G,%G,%G,%G,%G,%G,%G,%G,%G,%ld,%ld,%ld,%ld,%ld,%ld\r\n",
			tdFeat->RMS,
			tdFeat->VarianceVal,
			tdFeat->SigSkewnessVal,
			tdFeat->SigKurtosisVal,
			tdFeat->SigKurtosisVal,
			tdFeat->SigCrestFactor,
			tdFeat->SigShapeFactor,
			tdFeat->SigImpulseFactor,
			tdFeat->SigMarginFactor,
			fdFeat->PeakAmp1,
			fdFeat->PeakAmp2,
			fdFeat->PeakAmp3,
			fdFeat->PeakLocs1,
			fdFeat->PeakLocs2,
			fdFeat->PeakLocs3
			);
}

void formatFeaturestoString(char **bufferPtr, TDFeatures *tdFeat, FDFeatures *fdFeat) {
	createFormatedString(
			bufferPtr,
			"%G,%G,%G,%G,%G,%G,%G,%G,%G,%ld,%ld,%ld,%ld,%ld,%ld",
			tdFeat->RMS,
			tdFeat->VarianceVal,
			tdFeat->SigSkewnessVal,
			tdFeat->SigKurtosisVal,
			tdFeat->SigKurtosisVal,
			tdFeat->SigCrestFactor,
			tdFeat->SigShapeFactor,
			tdFeat->SigImpulseFactor,
			tdFeat->SigMarginFactor,
			fdFeat->PeakAmp1,
			fdFeat->PeakAmp2,
			fdFeat->PeakAmp3,
			fdFeat->PeakLocs1,
			fdFeat->PeakLocs2,
			fdFeat->PeakLocs3
			);
}

int createFormatedString(char **bufferPtr, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	// Calcula o tamanho necessario
	int len = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	if (len < 0) {
		return -1;  // Erro ao calcular o tamanho
	}

	// Aloca memoria suficiente para a string
	*bufferPtr = (char *)malloc(len + 1);  // +1 para o terminador nulo

	if (*bufferPtr == NULL) {
		return -1;  // Falha na alocacao de memoria
	}

	// Escreve a string formatada no buffer alocado
	va_start(args, fmt);
	int result = vsnprintf(*bufferPtr, len + 1, fmt, args);  // Escreve a string
	va_end(args);

	// Retorna o numero de caracteres escritos (sem o terminador nulo)
	return result;
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
//	int buffer[1024];
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

//void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) {
//
////	arm_copy_f32(&adcBuffer[0], &inputBuffer[HALF_INPUT_BUFFER_SIZE], OUTPUT_SIGNAL_SIZE);
//	for (int i = 0; i < OUTPUT_SIGNAL_SIZE; i++) {
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
////	arm_copy_f32(&adcBuffer[HALF_INPUT_BUFFER_SIZE], &inputBuffer[HALF_INPUT_BUFFER_SIZE], OUTPUT_SIGNAL_SIZE);
//	for (int i = 0; i < OUTPUT_SIGNAL_SIZE; i++) {
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
	float32_t features[] = { tdFeatures.RMS, tdFeatures.VarianceVal,
			tdFeatures.SigShapeFactor, tdFeatures.SigKurtosisVal,
			tdFeatures.SigSkewnessVal, tdFeatures.SigImpulseFactor,
			tdFeatures.SigCrestFactor, tdFeatures.SigMarginFactor,
			fdFeatures.PeakAmp1, fdFeatures.PeakAmp2, fdFeatures.PeakAmp3,
			fdFeatures.PeakLocs1, fdFeatures.PeakLocs2, fdFeatures.PeakLocs3 };
//	int errors = 0;
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
	float32_t features[] = { tdFeatures.RMS, tdFeatures.VarianceVal,
			tdFeatures.SigShapeFactor, tdFeatures.SigKurtosisVal,
			tdFeatures.SigSkewnessVal, tdFeatures.SigImpulseFactor,
			tdFeatures.SigCrestFactor, tdFeatures.SigMarginFactor,
			fdFeatures.PeakAmp1, fdFeatures.PeakAmp2, fdFeatures.PeakAmp3,
			fdFeatures.PeakLocs1, fdFeatures.PeakLocs2, fdFeatures.PeakLocs3 };

//	int errors = 0;
	char msg[80];
	const int32_t out = model_predict(features, n_features);

//	if (out != expect_result) {
//		printf("test-fail sample=%d expect=%d got=%d \r\n", i, expect_result, out);
//			errors += 1;
//	}

	sprintf(msg, "test extra_trees result=%ld \r\n", out);
	HAL_UART_Transmit(&huart2, (uint8_t*) msg, strlen(msg), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", strlen("\r\n"),
	HAL_MAX_DELAY);

	return out;
}

int32_t gaussian_naive_bayes_test(void) {
	const int n_features = 14;
//	const int n_testcases = testset_samples;
	float32_t features[] = { tdFeatures.RMS, tdFeatures.VarianceVal,
			tdFeatures.SigShapeFactor, tdFeatures.SigKurtosisVal,
			tdFeatures.SigSkewnessVal, tdFeatures.SigImpulseFactor,
			tdFeatures.SigCrestFactor, tdFeatures.SigMarginFactor,
			fdFeatures.PeakAmp1, fdFeatures.PeakAmp2, fdFeatures.PeakAmp3,
			fdFeatures.PeakLocs1, fdFeatures.PeakLocs2, fdFeatures.PeakLocs3 };

//	int errors = 0;
	char msg[80];
	const int32_t out = model_predict(features, n_features);

//	if (out != expect_result) {
//		printf("test-fail sample=%d expect=%d got=%d \r\n", i, expect_result, out);
//			errors += 1;
//	}

	sprintf(msg, "test gaussian_naive_bayes result=%ld \r\n", out);
	HAL_UART_Transmit(&huart2, (uint8_t*) msg, strlen(msg), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", strlen("\r\n"),
	HAL_MAX_DELAY);

	return out;
}

int32_t random_forest_test(void) {
	const int n_features = 14;
//	const int n_testcases = testset_samples;
	float32_t features[] = { tdFeatures.RMS, tdFeatures.VarianceVal,
			tdFeatures.SigShapeFactor, tdFeatures.SigKurtosisVal,
			tdFeatures.SigSkewnessVal, tdFeatures.SigImpulseFactor,
			tdFeatures.SigCrestFactor, tdFeatures.SigMarginFactor,
			fdFeatures.PeakAmp1, fdFeatures.PeakAmp2, fdFeatures.PeakAmp3,
			fdFeatures.PeakLocs1, fdFeatures.PeakLocs2, fdFeatures.PeakLocs3 };

//	int errors = 0;
	char msg[80];
	const int32_t out = model_predict(features, n_features);

//	if (out != expect_result) {
//		printf("test-fail sample=%d expect=%d got=%d \r\n", i, expect_result, out);
//		errors += 1;
//	}

	sprintf(msg, "test random_forest result=%ld \r\n", out);
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
