/*
 * FeatureExtraction.h
 *
 *  Created on: May 21, 2024
 *      Author: Italo LANZA
 * Description: Cabecalho file contendo as funcoes necessarias para extracao das
 * 				features utilizadas no projeto.
 */

#ifndef FEATUREEXTRACTION_H_
#define FEATUREEXTRACTION_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "arm_math.h"
/* Typedef -----------------------------------------------------------*/
//Time Domain Features
typedef struct {
	float32_t RMS;
	//float32_t MeanVal; 	/* Removed in the article */
	//float32_t MedianVal; 	/* Removed in the article */
	float32_t VarianceVal;
	float32_t SigSkewnessVal;
	float32_t SigKurtosisVal;
	float32_t SigShapeFactor;
	float32_t SigImpulseFactor;
	float32_t SigCrestFactor;
	float32_t SigMarginFactor;
} TDFeatures;

//Frequency Domain Features
typedef struct {
	int32_t PeakAmp1; // largest amplitude of the extracted frequencies in the signal
	int32_t PeakAmp2; // second largest amplitude of the extracted frequencies in the signal
	int32_t PeakAmp3; // third largest amplitude of the extracted frequencies in the signal
	int32_t PeakLocs1; 	// frequency value of the largest amplitude
	int32_t PeakLocs2; 	// frequency value of the second largest amplitude
	int32_t PeakLocs3; 	// frequency value of the third largest amplitude
} FDFeatures;

/* Functions prototypes ---------------------------------------------*/
void calculateVectorSquareRoot(float32_t *inputArray, float32_t *outputArray,
		uint32_t length);
float32_t calculateKurtosis(float32_t *signal, uint32_t length, float32_t mean,
		float32_t stdDev);
float32_t calculateSkewness(float32_t *signal, uint32_t length, float32_t mean,
		float32_t stdDev);
void extractTimeDomainFeatures(TDFeatures *tdFeatures, float32_t *buffer,
		uint16_t bufferSize);
void extractFrequencyDomainFeatures(FDFeatures *fdFeatures, float32_t *buffer,
		uint16_t bufferSize, uint32_t sampleRate);

#endif /* FEATUREEXTRACTION_H_ */
