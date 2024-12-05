/*
 * StringFormatter.h
 *
 *  Created on: Dec 4, 2024
 *      Author: italo
 */

#ifndef INC_STRINGFORMATTER_H_
#define INC_STRINGFORMATTER_H_

/* Includes ------------------------------------------------------------------*/
#include <stdarg.h>
#include "FeatureExtraction.h"

/* Typedef -------------------------------------------------------------------*/

/* Functions prototypes ------------------------------------------------------*/
void formatFeaturestoString(char **bufferPtr, TDFeatures *tdFeat, FDFeatures *fdFeat);
void formatFeaturesAndResultToString(char **bufferPtr, TDFeatures *tdFeat, FDFeatures *fdFeat, int32_t pResult);
int createFormatedString(char **bufferPtr, const char *fmt, ...);



#endif /* INC_STRINGFORMATTER_H_ */
