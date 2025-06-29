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
#include "StringFormatter.h"
#include "ModelSupportFunctions.h"

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
 * off.wav
 * health.wav
 * f1.wav
 * f2.wav
 * f3.wav
 */

#define AUDIO_FILE_NAME "off.wav"
//#define CSV_FILE_NAME "off.csv"
#define CSV_FILE_NAME "dt-f-off.csv"
//#define CSV_HEADER "RMS,Variance,Skewness,Kurtosis,CrestFactor,ShapeFactor,ImpulseFactor,MarginFactor,Peak1,Peak2,Peak3,PeakLocs1,PeakLocs2,PeakLocs3"
#define CSV_HEADER "RMS,Variance,Skewness,Kurtosis,CrestFactor,ShapeFactor,ImpulseFactor,MarginFactor,Peak1,Peak2,Peak3,PeakLocs1,PeakLocs2,PeakLocs3,Predicted"
//#define CSV_HEADER "TempoLeituraSD,TempoNormalizacao,TempoJanela,TempoFeatTempo,TempoFFT,TempoFeatFrequencia,TempoInferencia,TempoTotalBatch"
#define INPUT_BUFFER_SIZE 2048
#define HALF_INPUT_BUFFER_SIZE (INPUT_BUFFER_SIZE / 2)
#define SIGNAL_BUFFER_SIZE (INPUT_BUFFER_SIZE * 2)
#define OUTPUT_SIGNAL_SIZE (SIGNAL_BUFFER_SIZE / 2)
#define SAMPLE_RATE_HZ 48000 											//TODO: MUDAR DE ACORDO COM O VALOR REAL MOSTRADO PELO PROJETO
#define OVERLAP_FACTOR 0.75  											// Overlapping de 75%
#define ADVANCE_SIZE (int)(INPUT_BUFFER_SIZE * (1.0f - OVERLAP_FACTOR)) // 25% de avanco
#define NUM_STAGES 4 													// Filtro Ordem 8

#define INT16_TO_FLOAT (1.0f / (32768.0f))
//#define FLOAT_TO_INT16 32768.0f
#define INT12_TO_FLOAT (1.0 / (4096.0f))
#define FLOAT_TO_INT12 4096.0f

// Definir apenas uma dessas opcoes:
#define USE_SD_AUDIO           		// Usar audio do cartao SD
//#define USE_MIC_AUDIO             // Usar microfone (ADC + DMA)

// Verificação para garantir que apenas uma opcao esta ativa
#if defined(USE_SD_AUDIO) && defined(USE_MIC_AUDIO)
	#error "Voce deve definir apenas uma das opcoes: USE_SD_AUDIO ou USE_MIC_AUDIO"
#endif

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
// Variaveis comuns
static volatile int16_t inputBuffer[INPUT_BUFFER_SIZE] = { 0 };		// Buffer para armazenar os dados de entrada (ADC ou arquivo .wav)
static float32_t inputSignal[SIGNAL_BUFFER_SIZE] = { 0 };
static float32_t outputSignal[OUTPUT_SIGNAL_SIZE] = { 0 }; 			// Buffer de saída para os resultados da FFT
static float32_t hanningWindow[OUTPUT_SIGNAL_SIZE] = { 0 }; 		// Buffer para a janela de hanning
static float32_t data[INPUT_BUFFER_SIZE] = { 0 }; 					// Buffer de entrada da FFT (necessario porque a FFT afeta o vetor de entrada)
static float32_t dataFiltered[INPUT_BUFFER_SIZE] = { 0 }; 					// Buffer de entrada da FFT (necessario porque a FFT afeta o vetor de entrada)
//static float32_t fir_output_buffer[INPUT_BUFFER_SIZE] = { 0 };
//static volatile uint8_t fftBufferReadyFlag; 						// Variavel que informa se o buffer entrada da FFT esta pronto para processamento
arm_rfft_fast_instance_f32 fftHandler;								// Esturura para FFT real
//uint32_t deltaTimes[8] = { 0 };

arm_biquad_casd_df1_inst_f32 filterHandler;							// Instância da estrutura do filtro. Ela manterá todas as informações
static float32_t pState[NUM_STAGES * 2] = { 0 }; 					// Buffer de estado do filtro.

const float32_t NORM_FACTOR = 1.0f / 32767.0f;

char outputString[MAX_STRING_LENGTH]; 	// variavel para armazenar a string de saida

#ifdef USE_SD_AUDIO
int16_t volatile audioBuffer[INPUT_BUFFER_SIZE] = { 0 };	// Buffer para armazenar amostras de áudio lidas do arquivo WAV
FATFS FatFs; 												// Estrutura do FatFs
FIL inputFile;     											// Estrtura de um arquivo do FatFs (Arquivo WAV no cartão SD)
FIL outputFile;   											// Estrtura de um arquivo do FatFs (Arquivo csv no cartão SD)
FRESULT fres; 												// Estutura de resultado de uma operacao do FatFs
UINT bytesRead;                     						// Numero de bytes lidos do arquivo
WAVHeader wavHeader;										// Estura de um cabecalho de um arquivo .wav
uint32_t dataSize;											// Variavel para armazenar a quantidade de dados lida do sd card
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

// Coeficientes do Filtro Passa-Faixa Butter de 4 estagios (Ordem 8)
// Fs=48000Hz, Fc=[20.0Hz, 20000.0Hz]
static const float32_t pCoeffs[NUM_STAGES * 5] = {
    0.49803642f,    0.99607284f,    0.49803642f,    -1.18440148f,    -0.36786567f,
    1.00000000f,    2.00000000f,    1.00000000f,    -1.45394167f,    -0.67891696f,
    1.00000000f,    -2.00000000f,    1.00000000f,    +1.99516502f,    -0.99517187f,
    1.00000000f,    -2.00000000f,    1.00000000f,    +1.99799243f,    -0.99799927f,
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM2_Init(void);
static void MX_CRC_Init(void);
/* USER CODE BEGIN PFP */

// Prototipos relacionados a leitura de arquivos de audio no SDCard
#ifdef USE_SD_AUDIO
FRESULT readWAVHeader(FIL *file, WAVHeader *header);
FRESULT readWAVData(FIL *file, void *buffer, size_t numSamplesToRead,
		size_t *numBytesRead);
void printWAVHeader(const WAVHeader *header);
FRESULT readAllWavFile(FIL *file, WAVHeader *header);
#endif

// Pre-processamento
void createHanningWindow(float32_t *window, int size);

// Funcoes de suporte
void myprintf(const char *fmt, ...);
void printFeatures(TDFeatures *tdFeat, FDFeatures *fdFeat);

#ifdef USE_MIC_AUDIO
	void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc);
	void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc);
	static void ADC_Init(void);
	//static void MX_ADC1_Init(void);
	//static void MX_TIM2_Init(void);
#endif

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

	arm_rfft_fast_init_f32(&fftHandler, OUTPUT_SIGNAL_SIZE); // Inicializa Estrutura Handler da FFT
	arm_biquad_cascade_df1_init_f32(&filterHandler, NUM_STAGES, &pCoeffs[0], &pState[0]);
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
  MX_TIM2_Init();
  MX_CRC_Init();
  /* USER CODE BEGIN 2 */

	uint32_t sampleRate;
	size_t sampleSize;

#ifdef USE_SD_AUDIO
	MX_FATFS_Init();
	HAL_Delay(5000); 			// Um delay para o cartao SD estabilizar
	SDCard_Init(&FatFs);
	HAL_TIM_Base_Start(&htim2);	// Faz com que o Timer 2 comece a contar
	// Montagem do file system
	fres = SDCard_Mount();
	if (fres != FR_OK) {
		myprintf("[ERRO] Erro no SDCard_Mount. Codigo do erro: (%i)\r\n", fres);

		Error_Handler(); // loop infinito para travar a execucao do programa
	}

	// Tenta abrir o arquivo com o nome defino pela flag AUDIO_FILE_NAME
	fres = SDCard_OpenFile(&inputFile, AUDIO_FILE_NAME, FA_READ);
	if (fres != FR_OK) {
		myprintf("[ERRO] Erro ao abrir arquivo '%s'. Codigo do erro: (%i)\r\n",
		AUDIO_FILE_NAME, fres);

		SDCard_Unmount();

		Error_Handler(); // loop infinito para travar a execucao do programa
	}

	// Tenta criar o arquivo com o nome defino pela flag CSV_FILE_NAME
	fres = SDCard_OpenFile(&outputFile, CSV_FILE_NAME,
			FA_CREATE_ALWAYS | FA_WRITE);
	if (fres != FR_OK) {
		myprintf("[ERRO] Erro ao abrir arquivo '%s'. Codigo do erro: (%i)\r\n",
		CSV_FILE_NAME, fres);

		SDCard_CloseFile(&inputFile);
		SDCard_Unmount();

		Error_Handler(); // loop infinito para travar a execucao do programa
	}

	// Ler o cabeçalho do arquivo WAV
	fres = readWAVHeader(&inputFile, &wavHeader);
	if (fres != FR_OK) {
		// Lidar com erros na leitura do cabeçalho
		myprintf(
				"[ERRO] Erro ao abrir arquivo '.wav'. Codigo do erro: (%i)\r\n",
				fres);

		SDCard_CloseFile(&inputFile);
//		SDCard_CloseFile(&outputFile);
		SDCard_Unmount();

		Error_Handler(); // loop infinito para travar a execucao do programa
	}

	printWAVHeader(&wavHeader);

	// Verificar se é um arquivo WAV válido
	if (strncmp(wavHeader.chunkID, "RIFF", 4) == 0
			&& strncmp(wavHeader.format, "WAVE", 4) == 0) {

		// Acessando os campos do cabeçalho WAV
		sampleRate = wavHeader.sampleRate;
		sampleSize = (size_t) ((wavHeader.bitsPerSample * wavHeader.numChannels)
				/ 8);

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
//	arm_fill_f32(0.0f, inputSignal, OUTPUT_SIGNAL_SIZE); 	// Preenche o inputSignal com 0.0 inicialmente
//	arm_fill_f32(1.0f, hanningWindow, OUTPUT_SIGNAL_SIZE); 	// Preenche o hanningWindow com 1.0 inicialmente
	createHanningWindow(hanningWindow, OUTPUT_SIGNAL_SIZE); // Cria o vetor com a janela de Hann

	myprintf("\r\n~ Processando dados ~\r\n\r\n");

//	uint32_t startTick, startTime;
	SDCard_WriteLine(&outputFile, CSV_HEADER);

	while (1) {
#ifdef USE_SD_AUDIO

		size_t bytesToRead =
				(dataSize > (INPUT_BUFFER_SIZE * sampleSize)) ?
						(INPUT_BUFFER_SIZE * sampleSize) : dataSize;
//		myprintf("\r\n~ Lendo arquivo do cartao SD ~\r\n\r\n");

//		startTick = __HAL_TIM_GET_COUNTER(&htim2);

		fres = SDCard_Read(&inputFile, inputBuffer, bytesToRead, &bytesRead);

//		deltaTimes[0] = __HAL_TIM_GET_COUNTER(&htim2) - startTick; // Mede o tempo que demorou para fazer a leitura do arquivo csv

//		startTick = __HAL_TIM_GET_COUNTER(&htim2);
//		startTime = startTick;
		// Ler amostras de audio do arquivo .wav no cartão SD
		if (fres == FR_OK && bytesRead >= (INPUT_BUFFER_SIZE * sampleSize)) {
			// Converter as amostras de 16 bits para float32_t e normalizar
			for (int i = 0; i < INPUT_BUFFER_SIZE; i++) {
				// Copia os dados para segunda metade do inputSignal
//				inputSignal[INPUT_BUFFER_SIZE + i] = (float32_t) inputBuffer[i];
				inputSignal[INPUT_BUFFER_SIZE + i] = (float32_t) inputBuffer[i] * NORM_FACTOR; // Normalizacao de 16 bits para float
			}

//			deltaTimes[1] = __HAL_TIM_GET_COUNTER(&htim2) - startTick; //Mede o tempo que demorou para fazer a normalizacao

		} else {
			// Erro de leitura ou fim de arquivo
			//f_lseek(&inputFile, 44); 	// Reinicia o arquivo (ciclo)

			// Fecha os arquivos WAV, CSV e desmontar o sistema de arquivos
			myprintf("\r\n");
			SDCard_CloseFile(&inputFile);
			myprintf("[INFO] Fechando arquivo \"%s\". Codigo do erro: (%i)\r\n",
					AUDIO_FILE_NAME, fres);
			SDCard_CloseFile(&outputFile);
			myprintf("[INFO] Fechando arquivo \"%s\". Codigo do erro: (%i)\r\n",
					CSV_FILE_NAME, fres);
			SDCard_Unmount();
			myprintf("[INFO] Desmontando FatFs. Codigo do erro: (%i)\r\n",
					fres);
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
																	// pela janela
		// Processamento dos dados lidos (no buffer) necessário
		// Overlapping de 75% antes do janelamento
		for (int n = (int) (INPUT_BUFFER_SIZE * (1.0f - OVERLAP_FACTOR));
				n <= INPUT_BUFFER_SIZE; n = n + ADVANCE_SIZE) {


			// Copia dados do input buffer para array que vai ser utilizado para processamento
			arm_copy_f32(&inputSignal[n], &data[0], OUTPUT_SIGNAL_SIZE);
//			arm_copy_f32(&inputSignal[n], &fir_output_buffer[0], OUTPUT_SIGNAL_SIZE);


			/*---------------------------------------------------------------------------*/
			/* Pre-processamento --------------------------------------------------------*/
			/*---------------------------------------------------------------------------*/

	//		startTick = __HAL_TIM_GET_COUNTER(&htim2);
			// Aplica o filtro ao sinal de entrada
//			arm_fir_f32(&firHandler, &fir_output_buffer[0], &data[0], OUTPUT_SIGNAL_SIZE);
			arm_biquad_cascade_df1_f32(&filterHandler, &data[0], &dataFiltered[0],  OUTPUT_SIGNAL_SIZE);

			// Aplica a janela de hanning no buffer de entrada da fft
			arm_mult_f32(&dataFiltered[0], hanningWindow, &dataFiltered[0],	OUTPUT_SIGNAL_SIZE);

	//		deltaTimes[2] = __HAL_TIM_GET_COUNTER(&htim2) - startTick;  // mede o tempo gasto para fazer multiplicacao


			/*---------------------------------------------------------------------------*/
			/* Extracao das Features no Dominio do Tempo --------------------------------*/
			/*---------------------------------------------------------------------------*/

			//			myprintf("\r\n~ Extracao das Features no Dominio do Tempo ~\r\n\r\n");
//			startTick = __HAL_TIM_GET_COUNTER(&htim2);

			extractTimeDomainFeatures(&tdFeatures, &dataFiltered[0], INPUT_BUFFER_SIZE);

//			deltaTimes[3] = __HAL_TIM_GET_COUNTER(&htim2) - startTick;// Mede o tempo para extrair as
																	  // features no dominio do Tempo


			/*---------------------------------------------------------------------------*/
			/* Extracao das Features no Dominio da Frequencia ---------------------------*/
			/*---------------------------------------------------------------------------*/

			//			myprintf("\r\n~ Extracao das Features no Dominio da Frequencia ~\r\n\r\n");
			// Calcula fft usando a biblioteca da ARM
//			startTick = __HAL_TIM_GET_COUNTER(&htim2);

			arm_rfft_fast_f32(&fftHandler, &dataFiltered[0], &outputSignal[0], 0); // o ultimo argumento significa que nao queremos calcular a fft inversa

//			deltaTimes[4] = __HAL_TIM_GET_COUNTER(&htim2) - startTick;// Mede o tempo para calcular
																	  // a fft

//			startTick = __HAL_TIM_GET_COUNTER(&htim2);

			extractFrequencyDomainFeatures(&fdFeatures, outputSignal, OUTPUT_SIGNAL_SIZE, sampleRate);

//			deltaTimes[5] = __HAL_TIM_GET_COUNTER(&htim2) - startTick;// Mede o tempo para extrair as
			// features do dominio da Frequencia

			// [DEBUG]
//			printFeatures(&tdFeatures, &fdFeatures);


			/*---------------------------------------------------------------------------*/
			/* Faz Inferencia -----------------------------------------------------------*/
			/*---------------------------------------------------------------------------*/

			//			myprintf("\r\n~ Faz Inferencia ~\r\n\r\n");
//			startTick = __HAL_TIM_GET_COUNTER(&htim2);

//			int32_t result = run_inference(test_model(&tdFeatures, &fdFeatures));
			int32_t result = run_inference(&tdFeatures, &fdFeatures);

//			deltaTimes[6] = __HAL_TIM_GET_COUNTER(&htim2) - startTick;// Mede o tempo que e gasto para
																	  // fazer a inferencia
			/*---------------------------------------------------------------------------*/
			/* Escreve no Cartao SD -----------------------------------------------------*/
			/*---------------------------------------------------------------------------*/
//
//			formatFeaturestoString(&outputString, &tdFeatures, &fdFeatures);
			formatFeaturesAndResultToString(outputString, &tdFeatures, &fdFeatures, result);
//			formatTimeArrayToString(&outputString, deltaTimes);
			fres = SDCard_WriteLine(&outputFile, outputString);

			if (fres != FR_OK) {
				myprintf(
						"[ERRO] Erro ao escrever linha no arquivo '%s'. Codigo do erro: (%i)\r\n",
						CSV_FILE_NAME, fres);
				SDCard_CloseFile(&outputFile);
				SDCard_CloseFile(&inputFile);
				SDCard_Unmount();
				Error_Handler();
			}

//			myprintf("Inference result: %d\r\n", result);
			myprintf("."); // minha barra de progresso ?!
//			HAL_Delay(50);
		}


		// Atualiza a primeira metade do inputSignal para proxima janela de dados
		arm_copy_f32(&inputSignal[INPUT_BUFFER_SIZE], &inputSignal[0],
		OUTPUT_SIGNAL_SIZE);

//		deltaTimes[7] = __HAL_TIM_GET_COUNTER(&htim2) - startTime; // Mede o tempo total gasto pra
																   // processar as janela

//		myprintf("."); // minha barra de progresso ?!

//		formatTimeArrayToString(&outputString, deltaTimes);
//		fres = SDCard_WriteLine(&outputFile, outputString);
//
//		if (fres != FR_OK) {
//			myprintf(
//					"[ERRO] Erro ao escrever linha no arquivo '%s'. Codigo do erro: (%i)\r\n",
//					CSV_FILE_NAME, fres);
//			SDCard_CloseFile(&outputFile);
//			SDCard_CloseFile(&inputFile);
//			SDCard_Unmount();
//			Error_Handler();
//		}

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
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

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
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

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
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 0xffffffff;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

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
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
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
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
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
		window[n] = 0.5 - (0.5 * cos(2.0 * M_PI * n / (size - 1)));
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
//	RMS,Variance,Skewness,Kurtosis,CrestFactor,ShapeFactor,ImpulseFactor,MarginFactor,Peak1,Peak2,Peak3,PeakLocs1,PeakLocs2,PeakLocs3
	myprintf("%G,%G,%G,%G,%G,%G,%G,%G,%G,%G,%G,%G,%G,%G\r\n", tdFeat->RMS,
			tdFeat->VarianceVal, tdFeat->SigSkewnessVal, tdFeat->SigKurtosisVal,
			tdFeat->SigCrestFactor, tdFeat->SigShapeFactor,
			tdFeat->SigImpulseFactor, tdFeat->SigMarginFactor, fdFeat->PeakAmp1,
			fdFeat->PeakAmp2, fdFeat->PeakAmp3, fdFeat->PeakLocs1,
			fdFeat->PeakLocs2, fdFeat->PeakLocs3);
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

FRESULT readWAVData(FIL *file, void *buffer, size_t numSamplesToRead,
		size_t *numBytesRead) {
	// TODO: Consertar funcao para receber o cabecalho do arquivo .wav e calcular
	//       a quantidade de bytes a serem lidos conforme a quantidade de samples
	//	     que serao lidas
	// Ler dados do arquivo WAV
	size_t bytesToRead;

	return SDCard_Read(&inputFile, inputBuffer, bytesToRead, numBytesRead);
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

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
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
