/*
 * StringFormatter.c
 *
 *  Created on: Dec 4, 2024
 *      Author: italo
 */
#include "StringFormatter.h"


void formatFeaturestoString(char **bufferPtr, TDFeatures *tdFeat, FDFeatures *fdFeat) {
	//	RMS,Variance,Skewness,Kurtosis,CrestFactor,ShapeFactor,ImpulseFactor,MarginFactor,Peak1,Peak2,Peak3,PeakLocs1,PeakLocs2,PeakLocs3
	createFormatedString(
			bufferPtr,
			"%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f",
			tdFeat->RMS,
			tdFeat->VarianceVal,
			tdFeat->SigSkewnessVal,
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


void formatFeaturesAndResultToString(char **bufferPtr, TDFeatures *tdFeat, FDFeatures *fdFeat, int32_t pResult) {
	//	RMS,Variance,Skewness,Kurtosis,CrestFactor,ShapeFactor,ImpulseFactor,MarginFactor,Peak1,Peak2,Peak3,PeakLocs1,PeakLocs2,PeakLocs3
		createFormatedString(
				bufferPtr,
				"%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%ld",
				tdFeat->RMS,
				tdFeat->VarianceVal,
				tdFeat->SigSkewnessVal,
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
				fdFeat->PeakLocs3,
				pResult
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
