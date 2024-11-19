/*
 * FeatureExtraction.c
 *
 *  Created on: May 21, 2024
 *      Author: italo
 */

#include "FeatureExtraction.h"
#include <math.h>
#include <stdio.h>

float32_t calculateKurtosis(float32_t *signal, uint32_t length, float32_t mean,
		float32_t stdDev) {
	// Inicializa a kurtosis
	float32_t kurtosis = 0.0f;

	// Calcula a kurtosis usando a fórmula
	for (uint32_t i = 0; i < length; i++) {
		float32_t diff = signal[i] - mean;
		kurtosis += (diff * diff * diff * diff)
				/ (length * stdDev * stdDev * stdDev * stdDev);
	}

	return kurtosis;
}

float32_t calculateSkewness(float32_t *signal, uint32_t length, float32_t mean,
		float32_t stdDev) {
	// Inicializa o skewness
	float32_t skewness = 0.0f;

	// Calcula o skewness usando a fórmula
	for (uint32_t i = 0; i < length; i++) {
		float32_t diff = signal[i] - mean;
		skewness += (diff * diff * diff) / (length * stdDev * stdDev * stdDev);
	}

	return skewness;
}

void extractTimeDomainFeatures(TDFeatures *tdFeatures, float32_t *buffer,
		uint16_t bufferSize) {

	float32_t AbsSig[bufferSize];
	float32_t MaxValue, MinValue, MeanVal, MeanAbs, StdDevValue;
	uint32_t MaxValueIndex, MinValueIndex;

	/* Calculate Max Value*/
	arm_max_f32(buffer, bufferSize, &MaxValue, &MaxValueIndex);

	/* Calculate Min Value*/
	arm_min_f32(buffer, bufferSize, &MinValue, &MinValueIndex);

	/* Calculate Absolute Value*/
	arm_abs_f32(buffer, AbsSig, bufferSize);

	/* Calculate Mean of the Absolute Signal */
	arm_mean_f32(AbsSig, bufferSize, &MeanAbs);

	/* Calculate Mean Value */
	arm_mean_f32(buffer, bufferSize, &MeanVal);

	/* Calculate RMS */
	arm_rms_f32(buffer, bufferSize, &(tdFeatures->RMS));

	/* Calculate Variance */
	arm_var_f32(buffer, bufferSize, &(tdFeatures->VarianceVal));

	/* Calculate Standart Deviation Value */
	arm_std_f32(buffer, bufferSize, &StdDevValue);

	/* Calculate SigShape Value */
	tdFeatures->SigShapeFactor = (tdFeatures->RMS) / MeanAbs;

	/* Calculate Kurtosis */
	tdFeatures->SigKurtosisVal = calculateKurtosis(buffer, bufferSize, MeanVal,
			StdDevValue);

	/* Calculate Skewness */
	tdFeatures->SigSkewnessVal = calculateSkewness(buffer, bufferSize, MeanVal,
			StdDevValue);

	/* Calculate Impulse Factor */
	tdFeatures->SigImpulseFactor = MaxValue / MeanAbs;

	/* Calculate Crest Factor */
	tdFeatures->SigCrestFactor = MaxValue / (tdFeatures->RMS);

	/* Calculate Margin Factor */
	tdFeatures->SigMarginFactor = (MaxValue - MinValue) / (MeanVal);
}

void extractFrequencyDomainFeatures(FDFeatures *fdFeatures, float32_t *buffer,
		uint16_t bufferSize, uint32_t sampleRate) {
	//The buffer must be the FFT Buffer Output

	// Atribui zero para todos os valores da estrutura
	memset(fdFeatures, 0, sizeof(FDFeatures));

	//Compute absolute value of complex FFT results per frequency bin, get peak
	int32_t freqIndex = 4;
	//TODO: Verificar novamente se amplitude da posicao 2 continua alta
	for (int index = 4; index < bufferSize; index += 2) {
		float curVal = sqrtf(
				(buffer[index] * buffer[index])
						+ (buffer[index + 1] * buffer[index + 1]));

		if (curVal > fdFeatures->PeakAmp1) {
			fdFeatures->PeakAmp3 = fdFeatures->PeakAmp2;
			fdFeatures->PeakAmp2 = fdFeatures->PeakAmp1;
			fdFeatures->PeakAmp1 = curVal;
			fdFeatures->PeakLocs3 = fdFeatures->PeakLocs2;
			fdFeatures->PeakLocs2 = fdFeatures->PeakLocs1;
			fdFeatures->PeakLocs1 = (uint32_t) ((freqIndex / 2)
					* (sampleRate / ((float) bufferSize)));
		} else if (curVal > fdFeatures->PeakAmp2) {
			fdFeatures->PeakAmp3 = fdFeatures->PeakAmp2;
			fdFeatures->PeakAmp2 = curVal;
			fdFeatures->PeakLocs3 = fdFeatures->PeakLocs2;
			fdFeatures->PeakLocs2 = (uint32_t) ((freqIndex / 2)
					* (sampleRate / ((float) bufferSize)));
		} else if (curVal > fdFeatures->PeakAmp3) {
			fdFeatures->PeakAmp3 = curVal;
			fdFeatures->PeakLocs3 = (uint32_t) ((freqIndex / 2)
					* (sampleRate / ((float) bufferSize)));
		}

		freqIndex = freqIndex + 2;
	}
}
