/*
 * ModelSupportFunctions.c
 *
 *  Created on: Dec 20, 2024
 *      Author: italo
 */

#include "ModelSupportFunctions.h"
//#include "decision_tree_model.h"
//#include "extra_trees_model.h"
#include "gaussian_naive_bayes_model.h"
//#include "random_forest_model.h"


//int32_t run_inference(int32_t (*func)(void)) {
//
//	return func();
//}


int32_t run_inference(TDFeatures *tdFeatures, FDFeatures *fdFeatures) {

	const int n_features = 14;
//	const int n_testcases = testset_samples;
//	"RMS,Variance,Skewness,Kurtosis,CrestFactor,ShapeFactor,ImpulseFactor,MarginFactor,Peak1,Peak2,Peak3,PeakLocs1,PeakLocs2,PeakLocs3,FAULT_ID"
//	'RMS','Variance','Skewness','Kurtosis', 'CrestFactor','ShapeFactor','ImpulseFactor','MarginFactor', 'Peak1','Peak2','Peak3','PeakLocs1','PeakLocs2','PeakLocs3','FalutID'

	float32_t features[] = { tdFeatures->RMS, tdFeatures->VarianceVal,
			 tdFeatures->SigSkewnessVal, tdFeatures->SigKurtosisVal,
			 tdFeatures->SigCrestFactor, tdFeatures->SigShapeFactor,
			 tdFeatures->SigImpulseFactor, tdFeatures->SigMarginFactor,
			fdFeatures->PeakAmp1, fdFeatures->PeakAmp2, fdFeatures->PeakAmp3,
			fdFeatures->PeakLocs1, fdFeatures->PeakLocs2, fdFeatures->PeakLocs3 };
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


//int32_t test_model(TDFeatures *tdFeatures, FDFeatures *fdFeatures) {
//	const int n_features = 14;
////	const int n_testcases = testset_samples;
////	"RMS,Variance,Skewness,Kurtosis,CrestFactor,ShapeFactor,ImpulseFactor,MarginFactor,Peak1,Peak2,Peak3,PeakLocs1,PeakLocs2,PeakLocs3,FAULT_ID"
////	'RMS','Variance','Skewness','Kurtosis', 'CrestFactor','ShapeFactor','ImpulseFactor','MarginFactor', 'Peak1','Peak2','Peak3','PeakLocs1','PeakLocs2','PeakLocs3','FalutID'
//
//	float32_t features[] = { tdFeatures->RMS, tdFeatures->VarianceVal,
//			 tdFeatures->SigSkewnessVal, tdFeatures->SigKurtosisVal,
//			 tdFeatures->SigCrestFactor, tdFeatures->SigShapeFactor,
//			 tdFeatures->SigImpulseFactor, tdFeatures->SigMarginFactor,
//			fdFeatures->PeakAmp1, fdFeatures->PeakAmp2, fdFeatures->PeakAmp3,
//			fdFeatures->PeakLocs1, fdFeatures->PeakLocs2, fdFeatures->PeakLocs3 };
////	int errors = 0;
////	char msg[80];
//	const int32_t out = model_predict(features, n_features);
//
////	if (out != expect_result) {
////		printf("test-fail sample=%d expect=%d got=%d \r\n", i, expect_result, out);
////			errors += 1;
////	}
//
////	sprintf(msg, "test decision_tree result=%d \r\n", out);
////	HAL_UART_Transmit(&huart2, (uint8_t*) msg, strlen(msg), HAL_MAX_DELAY);
////	HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", strlen("\r\n"),
////	HAL_MAX_DELAY);
//
//	return out;
//}

//int32_t test_decision_tree(TDFeatures *tdFeatures, FDFeatures *fdFeatures) {
//	const int n_features = 14;
////	const int n_testcases = testset_samples;
//	float32_t features[] = { tdFeatures->RMS, tdFeatures->VarianceVal,
//			tdFeatures->SigShapeFactor, tdFeatures->SigKurtosisVal,
//			tdFeatures->SigSkewnessVal, tdFeatures->SigImpulseFactor,
//			tdFeatures->SigCrestFactor, tdFeatures->SigMarginFactor,
//			fdFeatures->PeakAmp1, fdFeatures->PeakAmp2, fdFeatures->PeakAmp3,
//			fdFeatures->PeakLocs1, fdFeatures->PeakLocs2, fdFeatures->PeakLocs3 };
////	int errors = 0;
////	char msg[80];
//	const int32_t out = model_predict(features, n_features);
//
////	if (out != expect_result) {
////		printf("test-fail sample=%d expect=%d got=%d \r\n", i, expect_result, out);
////			errors += 1;
////	}
//
////	sprintf(msg, "test decision_tree result=%d \r\n", out);
////	HAL_UART_Transmit(&huart2, (uint8_t*) msg, strlen(msg), HAL_MAX_DELAY);
////	HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", strlen("\r\n"),
////	HAL_MAX_DELAY);
//
//	return out;
//}

//int32_t test_extra_trees(TDFeatures *tdFeatures, FDFeatures *fdFeatures) {
//	const int n_features = 14;
////	const int n_testcases = testset_samples;
//	float32_t features[] = { tdFeatures->RMS, tdFeatures->VarianceVal,
//			tdFeatures->SigShapeFactor, tdFeatures->SigKurtosisVal,
//			tdFeatures->SigSkewnessVal, tdFeatures->SigImpulseFactor,
//			tdFeatures->SigCrestFactor, tdFeatures->SigMarginFactor,
//			fdFeatures->PeakAmp1, fdFeatures->PeakAmp2, fdFeatures->PeakAmp3,
//			fdFeatures->PeakLocs1, fdFeatures->PeakLocs2, fdFeatures->PeakLocs3 };
//
////	int errors = 0;
////	char msg[80];
//	const int32_t out = model_predict(features, n_features);
//
////	if (out != expect_result) {
////		printf("test-fail sample=%d expect=%d got=%d \r\n", i, expect_result, out);
////			errors += 1;
////	}
//
////	sprintf(msg, "test extra_trees result=%ld \r\n", out);
////	HAL_UART_Transmit(&huart2, (uint8_t*) msg, strlen(msg), HAL_MAX_DELAY);
////	HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", strlen("\r\n"),
////	HAL_MAX_DELAY);
//
//	return out;
//}

//int32_t test_gaussian_naive_bayes(TDFeatures *tdFeatures, FDFeatures *fdFeatures) {
//	const int n_features = 14;
////	const int n_testcases = testset_samples;
//	float32_t features[] = { tdFeatures->RMS, tdFeatures->VarianceVal,
//			tdFeatures->SigShapeFactor, tdFeatures->SigKurtosisVal,
//			tdFeatures->SigSkewnessVal, tdFeatures->SigImpulseFactor,
//			tdFeatures->SigCrestFactor, tdFeatures->SigMarginFactor,
//			fdFeatures->PeakAmp1, fdFeatures->PeakAmp2, fdFeatures->PeakAmp3,
//			fdFeatures->PeakLocs1, fdFeatures->PeakLocs2, fdFeatures->PeakLocs3 };
//
////	int errors = 0;
////	char msg[80];
//	const int32_t out = model_predict(features, n_features);
//
////	if (out != expect_result) {
////		printf("test-fail sample=%d expect=%d got=%d \r\n", i, expect_result, out);
////			errors += 1;
////	}
//
////	sprintf(msg, "test gaussian_naive_bayes result=%ld \r\n", out);
////	HAL_UART_Transmit(&huart2, (uint8_t*) msg, strlen(msg), HAL_MAX_DELAY);
////	HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", strlen("\r\n"),
////	HAL_MAX_DELAY);
//
//	return out;
//}

//int32_t test_random_forest(TDFeatures *tdFeatures, FDFeatures *fdFeatures) {
//	const int n_features = 14;
////	const int n_testcases = testset_samples;
//	float32_t features[] = { tdFeatures->RMS, tdFeatures->VarianceVal,
//			tdFeatures->SigShapeFactor, tdFeatures->SigKurtosisVal,
//			tdFeatures->SigSkewnessVal, tdFeatures->SigImpulseFactor,
//			tdFeatures->SigCrestFactor, tdFeatures->SigMarginFactor,
//			fdFeatures->PeakAmp1, fdFeatures->PeakAmp2, fdFeatures->PeakAmp3,
//			fdFeatures->PeakLocs1, fdFeatures->PeakLocs2, fdFeatures->PeakLocs3 };
//
////	int errors = 0;
////	char msg[80];
//	const int32_t out = model_predict(features, n_features);
//
////	if (out != expect_result) {
////		printf("test-fail sample=%d expect=%d got=%d \r\n", i, expect_result, out);
////		errors += 1;
////	}
//
////	sprintf(msg, "test random_forest result=%ld \r\n", out);
////	HAL_UART_Transmit(&huart2, (uint8_t*) msg, strlen(msg), HAL_MAX_DELAY);
////	HAL_UART_Transmit(&huart2, (uint8_t*) "\r\n", strlen("\r\n"),
////	HAL_MAX_DELAY);
//
//	return out;
//}
