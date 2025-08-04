import numpy as np
import matplotlib.pyplot as plt
from sklearn.datasets import load_iris
from sklearn.tree import DecisionTreeClassifier, plot_tree
from sklearn.ensemble import RandomForestClassifier # Usado para Bagged Trees
from sklearn.neighbors import KNeighborsClassifier
from sklearn.naive_bayes import GaussianNB
from sklearn.svm import SVC

# --- 1. Carregar e Preparar os Dados ---
# Carrega o dataset Iris
iris = load_iris()

filtro_duas_classes = iris.target != 2 # Remove a classe 2 (Virginica)
X = iris.data[filtro_duas_classes, :2]
y = iris.target[filtro_duas_classes]
nomes_classes_filtrados = iris.target_names[:-1] # Pega os nomes das duas primeiras classes

# --- 2. Definir os Modelos ---
# Instancia cada um dos modelos que serão comparados
# Usamos RandomForestClassifier como uma implementação popular e poderosa de Bagged Trees
modelos = [
    DecisionTreeClassifier(max_depth=2),
    RandomForestClassifier(n_estimators=100, random_state=42), # Bagged Trees
    KNeighborsClassifier(n_neighbors=7),
    GaussianNB(),
    SVC(kernel='poly', degree=2, gamma='auto') # Quadratic SVM
]

titulos = [
    'Árvore de Decisão (Decision Tree)',
    'Agrupamento de Árvores (Bagged Trees)',
    'k-Vizinhos Mais Próximos (KNN)',
    'Naïve Bayes Gaussiano',
    'SVM Quadrático'
]

# --- 3. Criar a Grade de Pontos (Meshgrid) para o Gráfico ---
# Define os limites do gráfico com uma pequena margem
x_min, x_max = X[:, 0].min() - 1, X[:, 0].max() + 1
y_min, y_max = X[:, 1].min() - 1, X[:, 1].max() + 1

# Cria uma grade de pontos com uma resolução de 0.02
xx, yy = np.meshgrid(np.arange(x_min, x_max, 0.02),
                     np.arange(y_min, y_max, 0.02))

# --- 4. Treinar Modelos e Gerar Gráficos Separados ---
# Itera sobre cada modelo para treinar e plotar
for modelo, titulo in zip(modelos, titulos):
    # Cria uma nova figura para cada modelo
    fig, ax = plt.subplots(figsize=(8, 6))

    # Treina o modelo com os dados
    modelo.fit(X, y)

    # Faz a predição para cada ponto na grade para desenhar a fronteira de decisão
    Z = modelo.predict(np.c_[xx.ravel(), yy.ravel()])
    Z = Z.reshape(xx.shape)

    # Plota a fronteira de decisão (áreas coloridas)
    ax.contourf(xx, yy, Z, alpha=0.4)

    # Plota os pontos de dados do dataset Iris
    scatter = ax.scatter(X[:, 0], X[:, 1], c=y, s=30, edgecolor='k')

    # Configurações do subplot
    ax.set_title(titulo, fontsize=20)
    ax.set_xlabel(iris.feature_names[0], fontsize=18)
    ax.set_ylabel(iris.feature_names[1], fontsize=18)

    # Adiciona uma legenda para as classes
    legend_elements = scatter.legend_elements()
    ax.legend(legend_elements[0], nomes_classes_filtrados, title="Classes")
    
    # Gera um nome de arquivo a partir do título
    nome_arquivo = titulo.lower().replace(' ', '_').replace('(', '').replace(')', '')
    
    # Salva o gráfico da fronteira de decisão
    plt.savefig(f'{nome_arquivo}_fronteira.pdf')
    print(f"Gráfico '{nome_arquivo}_fronteira.pdf' gerado com sucesso.")
    plt.close(fig) # Fecha a figura para liberar memória

    # --- Adição: Plotar a estrutura da Árvore de Decisão ---
    if isinstance(modelo, DecisionTreeClassifier):
        fig_tree, ax_tree = plt.subplots(figsize=(15, 10)) # Tamanho ajustado
        plot_tree(modelo,
                  feature_names=iris.feature_names[:2],
                  class_names=nomes_classes_filtrados, # Usa os nomes filtrados
                  filled=True,
                  ax=ax_tree,
                  fontsize=20) # Fonte ajustada
        ax_tree.set_title("Estrutura da Árvore de Decisão", fontsize=20)
        plt.savefig('estrutura_dt.pdf')
        print("Gráfico 'estrutura_dt.pdf' gerado com sucesso.")
        plt.close(fig_tree)
