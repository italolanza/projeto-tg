# README

Scritps:

+ `Main.ipynb` -> Faz a extracao das features dos arquivos de audio (todos) utilizando o script `ExtractFeature.py` e escreve em um arquivo de texto `.csv`;
+ `FeatureExtractor.ipynb` -> Faz extracao das features da mesma forma que o descrito no artigo. Basicamente adiciona o janelamento de 75% ao script `Main.ipynb`;
+ `TrainClassifier.ipynb` -> Faz o treinamento e avaliacao da acuracia dos classificadores utilizando um arquivo `.csv` gerado pelo notebook `FeatureExtractor.ipynb`. Tambem treina e gera os modelos que sao exportados utilizando a biblioteca `EMLEARN`;
+ `doFilterHP.py` -> Adaptacao para Python do script de mesmo nome escrito Matlab do autor do artigo que aplica um filtro de Banda Alta;
+ `ExtractFeatures.py` -> Adaptacao para Python do script escrito em Matlab do autor do artigo que faz a extracao das features com base em um arquivo de audio;
+ `Teste_converter_audio2wav.ipynb` -> Notebook simples que utiliza a biblioteca `soundfile` para converter um arquivo de audio de extensao `.flac` para `.wav`;
+ `Test_EMLEARN.ipynb` -> Notebook para testar o funcionamento da biblioteca **emlearn**. Treina modelos e gera os arquivos **`.h`** que podem ser utilizados no uControlador;
+ `SDCard.py` -> Faz o treinamento e testa acuracia dos classificadores utilizando um arquivo csv (SDCard.txt) que contem as features extraidas dos audios. Utiliza apenas um subset das features. _Serve mais para teste_.
  + Classificadores utilizados:
    + `Bagged Trees Ensemble`
    + `Quadratic SVM`
    + `Fine Decision Tree`
    + `Naïve Bayes`
    + `KNeighbors (KNN)`
+ `FeatureExtractor_uC.ipynb` -> Notebook que faz a extracao das features em Python da mesma forma que esta implementado no microncontrolador;
+ `FeatureAnalyzer.ipynb` -> Notebook Python que le os arquivos de features do uControlador e do Python e cria graficos dos valores para facilitar a visualizacao/analise;
+ `TimeDataAnalyzer.ipynb` -> Notebook que faz a analise dos dados de tempo gasto em diferentes partes do codigo que roda no uControlador;
+ `DataAnalyzer.ipynb` -> Notebook que faz a analise dos dados coletados features extraidas do microcontrolador e geras os graficos para serem utilizados no relatorio do TG;


## Como usar o poetry

- Voce precisa possuir o _poetry_ instalado na sua maquina: `pip install poetry`

### Instalando as dependecias com `poetry`

- Para instalar as dependencias do projeto (`pythonproject.toml`), e necessario rodar o comando

```shell
poetry install --no-root
```

### Criando o kernel Jupyter Notebook com o `poetry`

- Para criar um Kernel, execute o comando abaixo:

```shell
poetry run python -m ipykernel install --user --name="Projeto-tg" --display-name "Kernel TG"
```

### Abrindo o kernel

- Execute o comando abaixo para comecar a execucao ao servidor Jupyter.

```shell
poetry run jupyter notebook
```

- Depois abra o VSCode e selecione o Kernel criado previamente (`Kernel TG`).

## Link Repositorio Artigo Base

+ [Repositorio Artigo](https://github.com/RashadShubita/Fault-Detection-using-TinyML)
