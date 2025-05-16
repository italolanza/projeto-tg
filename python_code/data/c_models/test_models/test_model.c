#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Substitua pelo header do seu modelo gerado pelo emlearn
#include "decision_tree_model.h"

#define NUM_FEATURES 14
#define MAX_LINE_LEN 1024
#define CSV_HEADER "RMS,Variance,Skewness,Kurtosis,CrestFactor,ShapeFactor,ImpulseFactor,MarginFactor,Peak1,Peak2,Peak3,PeakLocs1,PeakLocs2,PeakLocs3,FAULT_ID"

//Time Domain Features
// typedef struct {
// 	float32_t RMS;
// 	//float32_t MeanVal; 	/* Removed in the article */
// 	//float32_t MedianVal; 	/* Removed in the article */
// 	float32_t VarianceVal;
// 	float32_t SigSkewnessVal;
// 	float32_t SigKurtosisVal;
// 	float32_t SigShapeFactor;
// 	float32_t SigImpulseFactor;
// 	float32_t SigCrestFactor;
// 	float32_t SigMarginFactor;
// } TDFeatures;

// //Frequency Domain Features
// typedef struct {
// 	float32_t PeakAmp1; // largest amplitude of the extracted frequencies in the signal
// 	float32_t PeakAmp2; // second largest amplitude of the extracted frequencies in the signal
// 	float32_t PeakAmp3; // third largest amplitude of the extracted frequencies in the signal
// 	float32_t PeakLocs1; 	// frequency value of the largest amplitude
// 	float32_t PeakLocs2; 	// frequency value of the second largest amplitude
// 	float32_t PeakLocs3; 	// frequency value of the third largest amplitude
// } FDFeatures;


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <arquivo.csv>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Erro ao abrir arquivo");
        return 1;
    }

    char line[MAX_LINE_LEN];
    int total = 0, correct = 0;

    // Pular cabeçalho
    if (!fgets(line, sizeof(line), file)) {
        fclose(file);
        fprintf(stderr, "Arquivo vazio\n");
        return 1;
    }

    while (fgets(line, sizeof(line), file)) {
        // Remover nova linha
        line[strcspn(line, "\n")] = 0;

        float features[NUM_FEATURES];
        int expected;
        char *token;
        int i = 0;

        // Extrair features
        token = strtok(line, ",");
        while (token != NULL && i < NUM_FEATURES) {
            features[i++] = atof(token);
            token = strtok(NULL, ",");
        }

        // Último token é o FAULT_ID
        if (token != NULL) {
            expected = atoi(token);
        } else {
            fprintf(stderr, "Linha incompleta: %s\n", line);
            continue;
        }

        // Fazer predição (ajuste conforme seu modelo)
        const int predicted = model_predict(features, NUM_FEATURES);

        // Imprimir resultado
        printf("Amostra %d - Previsto: %d, Real: %d\n", 
               total + 1, predicted, expected);
        
        if (predicted == expected) correct++;
        total++;
    }

    printf("\nAcurácia: %.2f%%\n", (correct * 100.0) / total);
    fclose(file);
    return 0;
}