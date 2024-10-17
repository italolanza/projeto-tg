# README

Scritps:

+ `Main.ipynb` -> Faz a extracao das features dos arquivos de audio (todos) utilizando o script `ExtractFeature.py` e escreve em um arquivo de texto `.csv`;
+ `doFilterHP.py` -> Adaptacao para Python do script de mesmo nome escrito Matlab do autor do artigo que aplica um filtro de Banda Alta;
+ `ExtractFeature.py` -> Adaptacao para Python do script escrito em Matlab do autor do artigo que faz a extracao das features com base em um arquivo de audio;
+ `Teste_converter_audio2wav.ipynb` -> Notebook simples que utiliza a biblioteca `soundfile` para converter um arquivo de audio de extensao `.flac` para `.wav`;
+ `TrainClassifier.ipynb` -> Faz o treinamento e avaliacao da acuracia dos classificadores utilizando um arquivo `.csv` gerado pelo notebook `Main.ipynb`;
+ `SDCard.py` -> Faz o treinamento e testa acuracia dos classificadores utilizando um arquivo csv (SDCard.txt) que contem as features extraidas dos audios.
    + Classificadores utilizados:
        + `Bagged Trees Ensemble`
        + `Quadratic SVM`
        + `Fine Decision Tree`
        + `Naïve Bayes`
        + `KNeighbors (KNN)`
+ `FeatureExtractor` -> Notebook que faz a extracao das features da mesma forma que esta implementado no microncontrolador

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

## Link Repositorio Artigo Base

+ [Repositorio Artigo](https://github.com/RashadShubita/Fault-Detection-using-TinyML)
