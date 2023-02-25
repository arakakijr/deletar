#include <stdio.h>
#include <stdlib.h>
#include <climits>
#include <algorithm>    // std::random_shuffle
#include <cstdlib>      // std::rand, std::srand
#include <vector>       // std::vector
#include <cstring>       // std::vector
#include <chrono>



#define MAX_SIZE 100
#define POP_SIZE 100
#define NUM_GENERATIONS 100000
#define CROSSOVER_RATE 0.8
#define MUTATION_RATE 0.2

#define LOWER_BOUND 0 // Ao mudar esse valor para maior, o fitness fica mais interessante. (bonus para solucoes melhores) Pode? Somente uma ideia.
#define ELITISTA 1

using namespace std;

void genetic_algorithm();
void read_instance(char *filename);
double objective_function(int solution[MAX_SIZE]);
double read_sol_input();
void initial_population();

int N=0;
int matrizDist [MAX_SIZE][MAX_SIZE];
int matrizFlow [MAX_SIZE][MAX_SIZE];


int population [POP_SIZE][MAX_SIZE];
int new_population [POP_SIZE][MAX_SIZE];
vector<float> chancesSelecao;
vector<int> paisSelecionados;

int bestSolution[MAX_SIZE];
double bestSolutionCost = INT_MAX;
int numGeracaoBestCost = 0;

void gerar_RWS();
int gira_RWS();
void crossover (int parent1[MAX_SIZE], int parent2[MAX_SIZE], int cruza1[MAX_SIZE], int cruza2[MAX_SIZE]);
void mutation (int cromossomo[MAX_SIZE]);

int random_int(int min, int max)
{
   return min + rand() % (max+1 - min);
}

int main(void){
	std::srand ( unsigned ( std::time(0) ) );
	FILE *file = NULL;
	FILE *saida = NULL;
	file = fopen("lista.txt", "r");
	saida = fopen("resultados.txt", "a");
	if( file == NULL ){
		printf("Erro na leitura do arquivo de instancias!\n");
		exit(1);
	}
	if( saida == NULL ){
		printf("Erro na escrita do arquivo de resultado!\n");
		exit(1);
	}
	int nInstances = 0;
	fscanf(file, " %d", &nInstances);
	fprintf(saida, "Usando P=%d. MAX_GENERATIONS=%d. Crossover=%f. Mutation=%f.LowerBound=%d.Elitista=%d\n", POP_SIZE, NUM_GENERATIONS, CROSSOVER_RATE, MUTATION_RATE,LOWER_BOUND,ELITISTA);
	fprintf(saida, "Instance\tCost\tGeracao\tCPU(ms)\n");
	for(int k = 0; k < nInstances; k++){
		char instance_name[100];
		fscanf(file, " %s", instance_name);
		printf("Abrindo a instancia: %s\n", instance_name);
		read_instance(instance_name);
		// Limpeza variáveis globais
		paisSelecionados.clear();
		bestSolutionCost = INT_MAX;

		//read_sol_input();
		auto start = chrono::high_resolution_clock::now();
		genetic_algorithm();
		auto end = chrono::high_resolution_clock::now();
		chrono::duration<double, std::milli> ms_double = end - start;
		fprintf(saida, "%s\t%f\t%d\t%f\n", instance_name, bestSolutionCost, numGeracaoBestCost, ms_double.count());
	}
	printf("Encerrando algoritmo genetico para o QAP\n");
	fclose(file);
	fclose(saida);
	exit(0);
}

vector<double> populacao_probabilidades;


void gerar_RWS(){
	//chancesSelecao;
	double totalFitness = 0.0;
	populacao_probabilidades.clear();

	for( int i=0;i < POP_SIZE; i++){
		totalFitness += (1.0 / (objective_function(population[i])-LOWER_BOUND)); // fitness
		//printf("Somando fitness %f\n", totalFitness);
	}

	for( int i=0;i < POP_SIZE; i++){
		populacao_probabilidades.push_back((1.0 / (objective_function(population[i])-LOWER_BOUND)) / totalFitness); // Nao to usando lower bound
		//printf("Chromosome %d tem custo %f with probability %f\n",i, objective_function(population[i]), populacao_probabilidades[i]);
	}
}

int gira_RWS(){
	double rndNumber = rand() / (double) RAND_MAX;
	double offset = 0.0;
	int pick = 0;
	//printf("Numero sorteado double: %f\n", rndNumber);

	for (int i = 0; i < POP_SIZE; i++) {
		offset += populacao_probabilidades.at(i);
		if (rndNumber < offset) {
			pick = i;
			break;
		}
	}

	//printf("Chromosome %d selected.\n", pick);
	return pick;
}

void genetic_algorithm(){
	initial_population();
	// Faz as gerações
	for(int k = 0; k < NUM_GENERATIONS; k++){
		// Seleciona os pais a partir de POP (seleciona numero de pais = o tamanho da POP)
		// Metodo RWS (Roulette Whell Selection)
		// Produz as chances da RWS
		// Gira RWS para selecionar.
		// seleciona P parents
		// 80% cruza
		// 20% vai direto para NEW_POP
		//printf("Iniciando geracao de numero = %d\n", k);
		gerar_RWS();
		vector<int> pais_selecionados;
		for(int w = 0; w < POP_SIZE; w++){
			pais_selecionados.push_back(gira_RWS());
		}

		// Cruza
		int numCruza = 0;
		for(numCruza = 0; numCruza < (POP_SIZE*CROSSOVER_RATE); numCruza+=2){
			int parent1 = pais_selecionados[numCruza];
			int parent2 = pais_selecionados[numCruza+1];
			//printf("Cruzando idx=%d e idx=%d. Botando resultado em new=%d,%d\n", parent1, parent2, numCruza, numCruza+1);
			crossover(population[parent1], population[parent2], new_population[numCruza], new_population[numCruza+1]);
			// Mutação
			//printf("iniciando mutation em idx=%d\n", numCruza);
			mutation(new_population[numCruza]);
			//printf("iniciando mutation em idx=%d\n", numCruza+1);
			mutation(new_population[numCruza+1]);
		}

		// Modo elitista copia a melhor solução para geração seguinte obrigatoriamente
		if( ELITISTA == 1 ){
			double melhorIndividuoCusto = INT_MAX;
			int melhorSolIt = 0;
			for(int w = 0; w < POP_SIZE; w++){
				double custo =objective_function(population[w]); 
				if( custo < melhorIndividuoCusto ){
					melhorIndividuoCusto = custo;
					melhorSolIt = w;
				}
			}
			memcpy(new_population[numCruza], population[melhorSolIt], N * sizeof(int));
			numCruza++;
		}
		
		// Copia direto de POP -> NEW_POP. Preenchendo população.
		while( numCruza < POP_SIZE ){
			//printf("Copiando diretamente cromossomo %d para pos %d\n", pais_selecionados[numCruza], numCruza);
			memcpy(new_population[numCruza], population[pais_selecionados[numCruza]], N * sizeof(int));
			numCruza++;
		}

		// Copia tudo de NEW_POP -> POP
		for(int i = 0; i < POP_SIZE; i++){
			//printf("Copiando POPS. idx=%d\n", i);
			memcpy(population[i], new_population[i], N * sizeof(int));
			// Avalia e salva o indivíduo
			double custo = objective_function(population[i]);
			if( custo < bestSolutionCost ){
				memcpy(bestSolution, population[i], N * sizeof(int));
				bestSolutionCost = custo;
				numGeracaoBestCost = k;
				printf("*** Achei uma nova melhor solucao de = %f\n\n", bestSolutionCost);
			}
		}
		// Faz os cruzamentos (exatamente 80% dos pais selecionados)
		// Passa o resto dos pais selecionados diretamente p/ NEW_POP
		// Copia de NEW_POP p/ POP
	}
	printf("Custo da solucao do GA: %f ou %f. (Geracao= %d / %d)\n", objective_function(bestSolution), bestSolutionCost, numGeracaoBestCost, NUM_GENERATIONS);
}

void initial_population(){
	vector<int> vetorInicial;
	for(int i = 0; i < N; i++){
		vetorInicial.push_back(i);
	}

	for(int i = 0; i < POP_SIZE; i++){
		std::random_shuffle ( vetorInicial.begin(), vetorInicial.end() );
		std::copy(vetorInicial.begin(), vetorInicial.end(), population[i]);
		printf("Gerei um individuo inicial:\n");
		for(int j = 0; j < N; j++){
			printf("%d ", population[i][j]);
		}
		printf("\n");
		printf("Custo dele: %f\n\n", objective_function(population[i]));
	}
}

void mutation (int cromossomo[MAX_SIZE]){
	for(int i = 0; i < N; i++){
		if( random_int(1, 10000) <= MUTATION_RATE * 10000.0 ){ // chance de 20%
			// swap de um cromossomo de lugar
			int j = random_int(0, N-1);
			//printf("Vai trocar %d e %d\n", cromossomo[i], cromossomo[j]);
			int swap = cromossomo[i];
			cromossomo[i] = cromossomo[j];
			cromossomo[j] = swap;
			//printf("Trocaram %d e %d\n", cromossomo[i], cromossomo[j]);
		}
	}
}

void crossover (int parent1[MAX_SIZE], int parent2[MAX_SIZE], int cruza1[MAX_SIZE], int cruza2[MAX_SIZE]){
	int inseridos1[N], inseridos2[N];
	for(int i = 0; i < N; i++){
		cruza1[i] = -1;
		cruza2[i] = -1;
		inseridos1[i] = 0;
		inseridos2[i] = 0;
	}

	// peega parte fixa
	int a = random_int(0, N-1);
	int b = random_int(0, N-1);
	if( b < a ){
		int swap = a;
		a = b;
		b = swap;
	}
	//printf("parte fixa: %d a %d\n", a, b);
	//printf("cromo 1:");
	//for(int i = 0; i < N; i++){ printf("%d ", parent1[i]); }
	//printf("\n");
	//printf("cromo 2:");
	//for(int i = 0; i < N; i++){ printf("%d ", parent2[i]); }
	//printf("\n");

	// Passa a parte fixa [a,b] do parent1 para cruza1; depois pega na ordem de parent2
	// Passa a parte fixa [a,b] do parent2 para cruza2; depois pega na ordem de parent1

	for(int i = a; i <= b; i++){
		//printf("copiando %d de parent1 e %d de parent2\n", parent1[i], parent2[i]);
		cruza1[i] = parent1[i];
		cruza2[i] = parent2[i];

		inseridos1[cruza1[i]] = 1;
		inseridos2[cruza2[i]] = 1;
		//printf("fim aqui\n");
	}
	//printf("aqui2\n");

	// Descobre os genes que faltam para cada cruza
	vector<int> falta1, falta2;
	for(int i = 0; i < N; i++){
		int ele = parent2[i];
		//printf("checking %d...\n", ele);
		if( inseridos1[ele] == 0 ){
			falta1.push_back(ele);
			
		}
		//else printf("NAO\n");
	}
	for(int i = 0; i < N; i++){
		int ele = parent1[i];
		//printf("checking2 %d...\n", ele);
		if( inseridos2[ele] == 0 ){
			falta2.push_back(ele);
		}
		//else printf("NAO\n");
	}

	// Insere os genes faltantes em ordem (0,a) e depois (b+1,N)
	int w = 0;
	for(int i = 0; i < a; i++){
		cruza1[i] = falta1.at(w);
		cruza2[i] = falta2.at(w);
		w++;
	}
	for(int i = b+1; i < N; i++){
		cruza1[i] = falta1.at(w);
		cruza2[i] = falta2.at(w);
		w++;
	}

	//printf("Resultado:\n\n");
	//printf("filho 1:");
	//for(int i = 0; i < N; i++){ printf("%d ", cruza1[i]); }
	//printf("\n");
	//printf("filho 2:");
	//for(int i = 0; i < N; i++){ printf("%d ", cruza2[i]); }
	//printf("\n");
	//printf("***********:\n\n");
}


double objective_function(int solution[MAX_SIZE]){
	double soma = 0.0;
	for(int i = 0; i < N; i++){
		int fac1 = solution[i];
		int loc1 = i;
		for(int j = 0; j < N; j++){
			if( i == j ) continue;
			int fac2 = solution[j];
			int loc2 = j;
			soma += (matrizFlow[fac1][fac2] * matrizDist[loc1][loc2]);
		}
	}
	return soma;
}


double read_sol_input(){
	printf("Digite a permutacao de N=%d (permutacao inicia de indices 1 e vai ate N).\n", N);
	int mySol[MAX_SIZE];
	for(int i = 0; i < N; i++){
		int var= 0;
		scanf(" %d", &var);
		var--;
		mySol[i]=var;
	}
	printf("Lida a solucao.\n");
	double custo = objective_function(mySol);
	printf("Custo da solucao: %f\n", custo);
	return custo;
}


void read_instance(char *filename){
	FILE *instanciaArq = fopen(filename, "r");
	if (instanciaArq == NULL){
		printf("Erro na leitura do aruqivo de instancia: %s\n", filename);
		exit(1);
	}
	printf("Lendo arquivo de instancia: %s\n", filename);
	fscanf(instanciaArq, " %d", &N);
	for(int i = 0; i < N; i++){
		for(int j = 0; j < N; j++){
			int num = 0;
			fscanf(instanciaArq, " %d", &num);
			matrizDist[i][j] = num;
		}
	}

	for(int i = 0; i < N; i++){
		for(int j = 0; j < N; j++){
			int num = 0;
			fscanf(instanciaArq, " %d", &num);
			matrizFlow[i][j] = num;
		}
	}

	/*
	printf("Imprindo a leitura, para teste:\n");
	for(int i = 0; i < N; i++){
		for(int j = 0; j < N; j++){
			printf("%d ", matrizDist[i][j]);
		}
		printf("\n");
	}
	printf("\n\n--------------------------------\n\n");
	for(int i = 0; i < N; i++){
		for(int j = 0; j < N; j++){
			printf("%d\t", matrizFlow[i][j]);
		}
		printf("\n");
	}*/

}