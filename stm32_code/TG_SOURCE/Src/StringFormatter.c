/*
 * StringFormatter.c
 *
 *  Created on: Dec 4, 2024
 *      Author: Italo LANZA
 */
#include "StringFormatter.h"
#include <stdio.h>

void formatFeaturestoString(char *bufferPtr, TDFeatures *tdFeat, FDFeatures *fdFeat) {
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


void formatFeaturesAndResultToString(char *bufferPtr, TDFeatures *tdFeat, FDFeatures *fdFeat, int32_t pResult) {
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


void formatTimeArrayToString(char *bufferPtr, uint32_t *tArray) {
	//	"TempoLeituraSD,TempoNormalizacao,TempoPreProcessamento,TempoFeatTempo,TempoFFT,TempoFeatFrequencia,TempoInferencia"
	createFormatedString(
					bufferPtr,
					"%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld",
					tArray[0],
					tArray[1],
					tArray[2],
					tArray[3],
					tArray[4],
					tArray[5],
					tArray[6],
					tArray[7]
					);
}


int createFormatedString(char *bufferPtr, const char *fmt, ...) {

    va_list args;
    va_start(args, fmt);

    int result = vsnprintf(bufferPtr, MAX_STRING_LENGTH, fmt, args);
    va_end(args);

    if (result < 0 || result >= MAX_STRING_LENGTH) {
        return -1;  // Buffer pequeno ou erro de formatação
    }

//    *bufferPtr = formatBuffer;  // Retorna o buffer estático
    return result;
}
