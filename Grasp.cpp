#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <random>
#include <limits>
#include <ctime>

using namespace std;

struct Cidade {
    int id;
    int x;
    int y;
};

// --- Função de leitura original ---
vector<int> extrairDadosLimpos(const string& nomeArquivo) {
    vector<int> dados;
    ifstream arquivo(nomeArquivo);
    string linha;
    if (!arquivo.is_open()) return dados; 
    
    while (getline(arquivo, linha)) {
        size_t pos = linha.find('#');
        if (pos != string::npos) linha = linha.substr(0, pos);
        
        stringstream ss(linha);
        int valor;
        while (ss >> valor) dados.push_back(valor);
    }
    arquivo.close();
    return dados;
}

// --- Função de Avaliação do Custo do TSP ---
int avaliar_tsp(const vector<int>& caminho, const vector<vector<int>>& matriz) {
    int custo = 0, n = caminho.size();
    for (int i = 0; i < n - 1; i++) {
        custo += matriz[caminho[i]][caminho[i+1]];
    }
    custo += matriz[caminho[n-1]][caminho[0]];
    return custo;
}

// --- FASE 1: CONSTRUÇÃO GULOSA ALEATÓRIA (LRC por Valor) ---
vector<int> construcao_grasp(const vector<vector<int>>& matriz, double alpha, mt19937& gen) {
    int n = matriz.size();
    vector<int> solucao;
    vector<bool> visitado(n, false);
    
    // Sorteia a cidade inicial
    uniform_int_distribution<> disInt(0, n - 1);
    int cidade_atual = disInt(gen);
    
    solucao.push_back(cidade_atual);
    visitado[cidade_atual] = true;
    
    while ((int)solucao.size() < n) {
        int c_min = numeric_limits<int>::max();
        int c_max = numeric_limits<int>::min();
        
        // 1. Identifica c_min e c_max na Lista de Candidatos (LC - cidades nao visitadas)
        for (int i = 0; i < n; i++) {
            if (!visitado[i]) {
                int custo = matriz[cidade_atual][i];
                if (custo < c_min) c_min = custo;
                if (custo > c_max) c_max = custo;
            }
        }
        
        // 2. Calcula o limiar da LRC: g(c) <= c_min + alpha * (c_max - c_min)
        double limiar = c_min + alpha * (c_max - c_min);
        
        // 3. Constroi a Lista Restrita de Candidatos (LRC)
        vector<int> LRC;
        for (int i = 0; i < n; i++) {
            if (!visitado[i] && matriz[cidade_atual][i] <= limiar) {
                LRC.push_back(i);
            }
        }
        
        // 4. Seleciona aleatoriamente um elemento da LRC
        uniform_int_distribution<> disLRC(0, (int)LRC.size() - 1);
        int escolhida = LRC[disLRC(gen)];
        
        solucao.push_back(escolhida);
        visitado[escolhida] = true;
        cidade_atual = escolhida;
    }
    
    return solucao;
}

// --- FASE 2: BUSCA LOCAL (2-Opt) ---
vector<int> busca_local(vector<int> solucao, const vector<vector<int>>& matriz) {
    bool melhorou = true;
    int custo_atual = avaliar_tsp(solucao, matriz);
    int n = solucao.size();
    
    while (melhorou) {
        melhorou = false;
        for (int i = 0; i < n - 1; i++) {
            for (int j = i + 1; j < n; j++) {
                // Inverte a subrota entre i e j
                reverse(solucao.begin() + i, solucao.begin() + j + 1);
                int custo_vizinho = avaliar_tsp(solucao, matriz);
                
                if (custo_vizinho < custo_atual) {
                    custo_atual = custo_vizinho;
                    melhorou = true;
                    break; // Primeiro de melhora (First Improvement)
                } else {
                    // Desfaz a inversao se nao melhorou
                    reverse(solucao.begin() + i, solucao.begin() + j + 1);
                }
            }
            if (melhorou) break;
        }
    }
    return solucao;
}

int main() {
    // Leitura dos dados usando a sua função original
    vector<int> dados = extrairDadosLimpos("cidadestsp.txt");

    if (dados.empty() || dados.size() % 3 != 0) {
        cout << "Erro: Certifique-se de que o arquivo 'cidadestsp.txt' esta na mesma pasta do executavel." << endl;
        return 1;
    }

    int numVertices = dados.size() / 3;
    vector<Cidade> cidades(numVertices);
    
    int index = 0;
    for (int i = 0; i < numVertices; ++i) {
        cidades[i].id = dados[index];
        cidades[i].x = dados[index + 1];
        cidades[i].y = dados[index + 2];
        index += 3;
    }

    // Criacao da matriz de distancias
    vector<vector<int>> matriz(numVertices, vector<int>(numVertices, 0));
    for (int i = 0; i < numVertices; ++i) {
        for (int j = 0; j < numVertices; ++j) {
            if (i != j) {
                double dx = cidades[i].x - cidades[j].x;
                double dy = cidades[i].y - cidades[j].y;
                matriz[i][j] = round(sqrt(dx * dx + dy * dy));
            }
        }
    }

    random_device rd;
    mt19937 gen(rd());

    // --- REQUISITOS DO EXERCÍCIO ---
    int GRASPMax = 100;  // (a) Criterio de parada por numero fixo de iteracoes
    double alpha = 0.2;  // (b) Valor estatico de alpha (0.0 = totalmente guloso, 1.0 = aleatorio)
    
    vector<int> s_best;
    int custo_best = numeric_limits<int>::max();


    clock_t inicio = clock();
    // --- LOOP DO GRASP ---
    for (int iter = 0; iter < GRASPMax; iter++) {
        // (c) Construcao com LRC por valor
        vector<int> s = construcao_grasp(matriz, alpha, gen);
        
        // Busca Local
        s = busca_local(s, matriz);
        
        int custo_s = avaliar_tsp(s, matriz);
        if (custo_s < custo_best) {
            s_best = s;
            custo_best = custo_s;
        }
    }

    // --- RESULTADO FINAL ---
    clock_t fim = clock();
    cout << "Melhor rota: ";
    for (int i = 0; i < numVertices; i++) cout << cidades[s_best[i]].id << " ";
    cout << "\nCusto: " << custo_best << "\n";
    cout << "Tempo: " << double(fim - inicio) / CLOCKS_PER_SEC << " s\n";



    return 0;
}