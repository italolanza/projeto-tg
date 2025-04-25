/*
 * FeatureExtraction.c
 *
 *  Created on: May 21, 2024
 *      Author: italo
 */

#include "FeatureExtraction.h"
#include <math.h>
#include <stdio.h>

#define INPUT_BUFFER_SIZE 2048
static float32_t AbsSig[INPUT_BUFFER_SIZE];
static float32_t SqrtAbsSig[INPUT_BUFFER_SIZE];


void calculateVectorSquareRoot(float32_t *inputArray, float32_t *outputArray, uint32_t length) {
	for (uint32_t i = 0; i < length; i++) {
	    arm_sqrt_f32(inputArray[i], &outputArray[i]);
	}
}

float32_t calculateKurtosis(float32_t *signal, uint32_t length, float32_t mean,
		float32_t stdDev) {
	// Inicializa a kurtosis
	float32_t kurtosis = 0.0f;

	if (stdDev == 0 || length == 0) {
		return 0.0f;
	}

	// Calcula a kurtosis usando a fórmula
	for (uint32_t i = 0; i < length; i++) {
		float32_t diff = signal[i] - mean;
		kurtosis += (diff * diff * diff * diff);
	}

	kurtosis = kurtosis / (length * stdDev * stdDev * stdDev * stdDev);

	return kurtosis;
}

float32_t calculateSkewness(float32_t *signal, uint32_t length, float32_t mean,
		float32_t stdDev) {
	// Inicializa o skewness
	float32_t skewness = 0.0f;

	if (stdDev == 0 || length == 0) {
		return 0.0f;
	}

	// Calcula o skewness usando a fórmula
	for (uint32_t i = 0; i < length; i++) {
		float32_t diff = signal[i] - mean;
		skewness += (diff * diff * diff);
	}

	skewness = skewness / (length * stdDev * stdDev * stdDev);

	return skewness;
}

void extractTimeDomainFeatures(TDFeatures *tdFeatures, float32_t *buffer,
		uint16_t bufferSize) {

//	static float32_t AbsSig[bufferSize], SqrtAbsSig[bufferSize];
	float32_t MaxValue, MinValue, MeanVal, MeanAbs, MeanSqrtAbs, StdDevValue;
	uint32_t MaxValueIndex, MinValueIndex;

	/* Calculate Max Value*/
	arm_max_f32(buffer, bufferSize, &MaxValue, &MaxValueIndex);

	/* Calculate Min Value*/
	arm_min_f32(buffer, bufferSize, &MinValue, &MinValueIndex);

	/* Calculate Absolute Value*/
	arm_abs_f32(buffer, AbsSig, bufferSize);

	/* Calculate Square Root of the Absolute Signal  */
	calculateVectorSquareRoot(AbsSig, SqrtAbsSig, bufferSize);

	/* Calculate Mean of the Absolute Signal */
	arm_mean_f32(AbsSig, bufferSize, &MeanAbs);

	/* Calculate Mean of the Square Root of the Absolute Signal */
	arm_mean_f32(SqrtAbsSig, bufferSize, &MeanSqrtAbs);

	/* Calculate Mean Value */
	arm_mean_f32(buffer, bufferSize, &MeanVal);

	/* Calculate RMS */
	arm_rms_f32(buffer, bufferSize, &(tdFeatures->RMS));

	/* Calculate Variance */
	arm_var_f32(buffer, bufferSize, &(tdFeatures->VarianceVal));

	/* Calculate Standard Deviation Value */
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
	tdFeatures->SigMarginFactor = (MaxValue - MinValue)
			/ (MeanSqrtAbs * MeanSqrtAbs);
}

void extractFrequencyDomainFeatures(FDFeatures *fdFeatures, float32_t *buffer,
		uint16_t bufferSize, uint32_t sampleRate) {

	// Atribui zero para todos os valores da estrutura
	memset(fdFeatures, 0, sizeof(FDFeatures));

	/* Calcula vetor de Magnitudes */
//	float32_t fftMag[bufferSize/2];
//	arm_cmplx_mag_f32(buffer, fftMag, (bufferSize/2));
//
//	for (int index = 0; index < bufferSize / 2; index += 1) {
//		float curVal = fftMag[index];
//
//		if (curVal > fdFeatures->PeakAmp1) {
//			fdFeatures->PeakAmp3 = fdFeatures->PeakAmp2;
//			fdFeatures->PeakAmp2 = fdFeatures->PeakAmp1;
//			fdFeatures->PeakAmp1 = curVal;
//			fdFeatures->PeakLocs3 = fdFeatures->PeakLocs2;
//			fdFeatures->PeakLocs2 = fdFeatures->PeakLocs1;
//			fdFeatures->PeakLocs1 = (uint32_t) (index
//					* (sampleRate / ((float) bufferSize)));
//		} else if (curVal > fdFeatures->PeakAmp2) {
//			fdFeatures->PeakAmp3 = fdFeatures->PeakAmp2;
//			fdFeatures->PeakAmp2 = curVal;
//			fdFeatures->PeakLocs3 = fdFeatures->PeakLocs2;
//			fdFeatures->PeakLocs2 = (uint32_t) (index
//					* (sampleRate / ((float) bufferSize)));
//		} else if (curVal > fdFeatures->PeakAmp3) {
//			fdFeatures->PeakAmp3 = curVal;
//			fdFeatures->PeakLocs3 = (uint32_t) (index
//					* (sampleRate / ((float) bufferSize)));
//		}
//	}

	/* Metodo alternativo - sem precisar de um vetor auxiliar para a magnitude */

	for (int index = 0; index < bufferSize; index += 2) {

        float32_t real = buffer[index];
        float32_t imag = buffer[index + 1];
        float32_t mag_sq = real * real + imag * imag; // Magnitude ao quadrado

        // Atualiza picos (usando magnitude quadrada para comparação)
		if (mag_sq > fdFeatures->PeakAmp1) {
			fdFeatures->PeakAmp3 = fdFeatures->PeakAmp2;
			fdFeatures->PeakAmp2 = fdFeatures->PeakAmp1;
			fdFeatures->PeakAmp1 = mag_sq;
			fdFeatures->PeakLocs3 = fdFeatures->PeakLocs2;
			fdFeatures->PeakLocs2 = fdFeatures->PeakLocs1;
			fdFeatures->PeakLocs1 = (float32_t) ((index)
					* (sampleRate / ((float32_t) bufferSize)));
		} else if (mag_sq > fdFeatures->PeakAmp2) {
			fdFeatures->PeakAmp3 = fdFeatures->PeakAmp2;
			fdFeatures->PeakAmp2 = mag_sq;
			fdFeatures->PeakLocs3 = fdFeatures->PeakLocs2;
			fdFeatures->PeakLocs2 = (float32_t) ((index)
					* (sampleRate / ((float32_t) bufferSize)));
		} else if (mag_sq > fdFeatures->PeakAmp3) {
			fdFeatures->PeakAmp3 = mag_sq;
			fdFeatures->PeakLocs3 = (float32_t) ((index)
					* (sampleRate / ((float32_t) bufferSize)));
		}

	}

    // Converte magnitude quadrada para amplitude real
    arm_sqrt_f32(fdFeatures->PeakAmp1, &fdFeatures->PeakAmp1);
    arm_sqrt_f32(fdFeatures->PeakAmp2, &fdFeatures->PeakAmp2);
    arm_sqrt_f32(fdFeatures->PeakAmp3, &fdFeatures->PeakAmp3);
}
