/*
 * ModelSupportFunctions.h
 *
 *  Created on: Dec 20, 2024
 *      Author: Italo LANZA
 * Description: Header file contendo funcoes que auxiliam a execucao dos
 *              modelos treinados.
 *
 */

#ifndef INC_MODELSUPPORTFUNCTIONS_H_
#define INC_MODELSUPPORTFUNCTIONS_H_

/* Includes ------------------------------------------------------------------*/
#include "FeatureExtraction.h"


/* Typedef -----------------------------------------------------------*/
/* Functions prototypes ---------------------------------------------*/
//int32_t run_inference(int32_t (*func)(void));
int32_t run_inference(TDFeatures *tdFeatures, FDFeatures *fdFeatures);
//int32_t test_model(TDFeatures *tdFeatures, FDFeatures *fdFeatures);
//int32_t test_decision_tree(TDFeatures *tdFeatures, FDFeatures *fdFeatures);
//int32_t test_extra_trees(TDFeatures *tdFeatures, FDFeatures *fdFeatures);
//int32_t test_gaussian_naive_bayes(TDFeatures *tdFeatures, FDFeatures *fdFeatures);
//int32_t test_random_forest(TDFeatures *tdFeatures, FDFeatures *fdFeatures);


#endif /* INC_MODELSUPPORTFUNCTIONS_H_ */
