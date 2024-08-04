#include "pf1.h"

pthread_t *worker_threads;
params_t *thread_params;
stats_t *thread_stats;
sem_t mutex;
string **threads_strings;
FILE **available_files;
FILE **new_available_files;
int current_tmp_file;
int max_available;

// 22. Función que libera la memoria reservada en main

void free_threads_data (int argc) {
	free(threads_strings);
	free(thread_params);
	free(thread_stats);
	free(worker_threads);
}

// 20. Funcion que imprime la data final

void print_final_data (int argc) {
	int total_lines = 0;
	string longest = NULL;
	string shortest = NULL;
	for(int i = 0; i < argc - 1; i++) {
		total_lines += thread_stats[i].number_of_lines;
	}
	for(int i = 0; i < argc - 1; i++){
		if(thread_stats[i].shortest_line == NULL){
			continue;
		}
		if(shortest == NULL) {
			shortest = thread_stats[i].shortest_line;
			continue;
		}
		if(strlen(thread_stats[i].shortest_line) < strlen(shortest)) {
			shortest = strdup(thread_stats[i].shortest_line);
		} else if (strlen(shortest) == strlen(thread_stats[i].shortest_line)) {
			if(strcasecmp(shortest,thread_stats[i].shortest_line) < 0) {
				shortest = strdup(thread_stats[i].shortest_line);
			}
		}
	}
	for(int i = 0; i < argc - 1; i++){
		if(thread_stats[i].longest_line == NULL){
			continue;
		}
		if(longest == NULL) {
			longest = thread_stats[i].longest_line;
			continue;
		}
		if(strlen(thread_stats[i].longest_line) > strlen(longest)) {
			longest = strdup(thread_stats[i].longest_line);
		} else if (strlen(longest) == strlen(thread_stats[i].longest_line)) {
			if(strcasecmp(longest,thread_stats[i].longest_line) < 0) {
				longest = strdup(thread_stats[i].longest_line);
			}
		}
	}
	if(longest == NULL || shortest == NULL) {
		printf("A total of 0 strings were passed as input,\n"); 
		printf("longest string sorted: \n");
		printf("shortest string sorted: \n");
	} else {
		printf("A total of %d strings were passed as input,\n", total_lines); 
		printf("longest string sorted: %s\n", longest);
		printf("shortest string sorted: %s\n", shortest);
	}
}

// 21. Función que escribe el archivo final

void write_sorted_file () {
	FILE* sorted;
	size_t n = 0;
	sorted = fopen("sorted.txt", "w");
	string line;
	if(sorted == NULL){
		printf("ERROR (write_sorted_file): No se pudo crear el archivo \"sorted\"\n");
		pthread_exit(NULL);
	}
	for(int i = 0; getline(&line,&n,available_files[0]) != -1; i++){
		fprintf(sorted,"%s",line);
	}
	fclose(sorted);
}

// 20. Función que escribe todo el contenido de un archivo temporal a otro

void transfer_data (FILE* file1, FILE* file2){
	size_t n = 0;
	string line;
	if(file1 == NULL || file2 == NULL){
		printf("ERROR (transfer_data): No se pudo transferir la data de los archivos\n");
		pthread_exit(NULL);
	}
	rewind(file1);
	rewind(file2);
	for(int i = 0; getline(&line,&n,file1) != -1; i++){
		fprintf(file2,"%s",line);
	}
	rewind(file1);
	rewind(file2);
}

// 19. Función que cierra temporales

void next_level_files (int max, int odd) {
	if(odd == 0){
		for(int i = 0; i < max; i++){
			available_files[i] = tmpfile();
			transfer_data(new_available_files[i], available_files[i]);
		}
	} else {
		for(int i = 0; i < max; i++){
			available_files[i] = tmpfile();
			transfer_data(new_available_files[i], available_files[i]);
		}
		available_files[max] = available_files[max_available-1];
	}
}

// 18. Función que espera a los hilos 'fusionadores'

void wait_merge_threads (int number_of_threads) {
	for(int i = 0; i < number_of_threads; i++){
		pthread_join(worker_threads[i],NULL);
	}
}

// 16. Comparador por alfabeto

int compare_by_alphabet(const void* p, const void* q) { 
    string str_1 = *(const string*) p; 
    string str_2 = *(const string*) q; 
	
	// str_1 es menor que str_2
	if(strcasecmp(str_1,str_2) < 0){
		// str_2 debe ir primero que str_1
		return 1;
	}

	// str_1 es idéntico a str_2
	if(strcasecmp(str_1,str_2) == 0){
		// son equivalentes
		return 0;
	}

	// str_1 es mayor que str_2
	if(strcasecmp(str_1,str_2) > 0){
		// str_1 debe ir primero que str_2
		return -1;
	}

	/*
    ( < 0 ): Less than zero, if the element pointed by p1 goes before the element pointed by p2.
    ( 0 ): Zero, if the element pointed by p1 is equivalent to the element pointed by p2.
    ( > 0 ): Greater than zero, if the element pointed by p1 goes after the element pointed by p2.
	*/
}

// 15. delete_duplicates: Esta función elimina los duplicados de un string.

void delete_duplicates (string* strings, int length){
	for(int i = 0; i < length; i++) {
		for(int j = 0; j < length; j++){
			if(j != i && strcasecmp(strings[i],strings[j]) == 0 && strcasecmp(strings[j],"") != 0) {
				strings[j] = "";
			}
		}
	}
}

// 14. Contador de líneas para archivos temporales ya abiertos

int count_tmp_lines (FILE* file) {
	rewind(file);
	size_t n = 0;
	string line = NULL;
	int number_of_lines = 0;
	while(getline(&line,&n,file) != -1){
		number_of_lines++;
	}
	free(line);
	rewind(file);
	return number_of_lines;
}

// 13. Función del hilo 'fusionador'

void *merge_function (void* arg) {
	long fileID = (long) arg;
	FILE* file1 = available_files[fileID];
	FILE* file2 = available_files[fileID + 1];
	int lines1 = count_tmp_lines(file1);
	int lines2 = count_tmp_lines(file2);
	int total = lines1 + lines2;
	int curr = 0;
	string* strings = (string*) malloc(sizeof(string)*total);
	string line;
	size_t n = 0;
	for(int i = 0; getline(&line,&n,file1) != -1; i++){
		strings[i] = strdup(line);
	}
	for(int i = lines1; getline(&line,&n,file2) != -1; i++){
		strings[i] = strdup(line);
	}
	qsort((void *) strings, total, sizeof(string), compare_by_alphabet);
	sem_wait(&mutex);
		if((new_available_files[current_tmp_file] = tmpfile()) == NULL){
			printf("ERROR (merge_function): No se pudo crear el archivo temporal");
		}
		curr = current_tmp_file;
		current_tmp_file++;
	sem_post(&mutex);
	delete_duplicates(strings, total);
	for(int i = 0; i < total; i++){
		fprintf(new_available_files[curr], "%s", strings[i]);
	}
	total = count_tmp_lines(new_available_files[curr]);
	printf("Merged %d lines and %d lines into %d lines\n", lines1, lines2, total);
	rewind(new_available_files[curr]);

	fclose(file1);
	fclose(file2);	
}

// 12. Función que crea los hilos 'fusionadores' y les asigna dos archivos

void create_merge_threads (int number_of_threads) {
	long fileID = 0;
	current_tmp_file = 0;
	for(int i = 0; i < number_of_threads; i++){
		pthread_create(&worker_threads[i],NULL,merge_function,(void*) fileID);
		fileID += 2;
	}
}

// 11. Funcion principal para fusionar archivos .sorted

void init_merge () {
	sem_init(&mutex,0,1);
	while(max_available != 1){
		create_merge_threads(max_available/2);
		wait_merge_threads(max_available/2);
		if(max_available%2 == 0){
			next_level_files(max_available/2,0);
			max_available = max_available/2;
		} else {
			next_level_files(max_available/2,1);
			max_available = max_available/2 + 1;
		}
	}
	write_sorted_file();
}

// 10. Función que abre todos los archivos .sorted para los hilos 'fusionadores'

void open_sorted_files (int argc, char* argv[]) {
	available_files = (FILE**) malloc(sizeof(FILE*)*argc-1);
	new_available_files = (FILE**) malloc(sizeof(FILE*)*argc-1);
	max_available = argc-1;
	string url;
	for(int i = 1; i < argc; i++){
		url = strdup(argv[i]);
		strcat(url,".sorted");
		available_files[i - 1] = fopen(url,"r");
		if(available_files[i - 1] == NULL){
			printf("ERROR (open_sorted_files): No se pudo abrir '%s'\n",url);
			exit(1);
		}
	}
}

// PARTE I Y PARTE II

// 9. Función que espera a que los hilos trabajadores culminen

void wait_for_workers (int argc) {
	for (int i = 0; i < argc - 1; i++) {
		pthread_join (worker_threads[i],NULL);
	}
}

// 8. Función que libera los strings de un hilo

void free_thread (params_t *params) {
	int number_of_lines = thread_stats[params->id].number_of_lines;
	for(int i = 0; i < number_of_lines; i++){
		free(threads_strings[params->id][i]);
	}
	free(threads_strings[params->id]);
}

// 7. Función que almacena los datos del hilo en su posición del arreglo stats

void save_thread_data (params_t *params) {
	int number_of_lines = thread_stats[params->id].number_of_lines;

	if(number_of_lines == 0) {
		thread_stats[params->id].shortest_line = NULL;
		thread_stats[params->id].longest_line = NULL;
		return;
	}

	string shortest = threads_strings[params->id][0];
	string longest = threads_strings[params->id][0];

	for(int i = 0; i < number_of_lines; i++){
		if(strlen(shortest) > strlen(threads_strings[params->id][i])) {
			shortest = threads_strings[params->id][i];
		} else if (strlen(shortest) == strlen(threads_strings[params->id][i])){
			if(strcasecmp(shortest,threads_strings[params->id][i]) < 0) {
				shortest = threads_strings[params->id][i];
			}
		}
	}

	for(int i = 0; i < number_of_lines; i++){
		if(strlen(longest) < strlen(threads_strings[params->id][i])) {
			longest = threads_strings[params->id][i];
		} else if (strlen(longest) == strlen(threads_strings[params->id][i])) {
			if(strcasecmp(longest,threads_strings[params->id][i]) < 0) {
				longest = threads_strings[params->id][i];
			}
		}
	}

	thread_stats[params->id].shortest_line = strdup(shortest);
	thread_stats[params->id].longest_line = strdup(longest);
}

// 6. Función que ordena los strings de un hilo y los almacena en un archivo .sorted

void store_lines (params_t *params) {
	FILE *file;
	size_t n = 0;
	string sorted_filename = strdup(params->filename);
	int number_of_lines = thread_stats[params->id].number_of_lines;
	strcat(sorted_filename,".sorted");
	file = fopen(sorted_filename, "w");
	if(file == NULL){
		printf("ERROR (store_lines): No se pudo abrir '%s'\n",sorted_filename);
		pthread_exit(NULL);
	}
	qsort((void *) threads_strings[params->id], number_of_lines, sizeof(string), compare_by_alphabet);
	for(int i = 0; i < number_of_lines; i++){
		fprintf(file,"%s\n",threads_strings[params->id][i]);
	}
	fclose(file);
}

string trim(string str) {
	if(str==NULL){
		return '\0';
	}
	string trimmed;
	int index = 0;
	int auxIndex = 0;
	while(str[auxIndex] != '\0'){
		if(isspace(str[auxIndex]) == 0){
			auxIndex++;
			index=auxIndex;
		} else {
			auxIndex++;
		}
	}
	trimmed = (string) malloc(sizeof(char)*(index+1));
	strncpy(trimmed, str, index);
	trimmed[index] = '\0';
	return trimmed;
}

void read_lines (params_t *params) {
	FILE *file;
	size_t strings_amount = 50;
	size_t string_size = 200;
	int number_of_lines = 0;
	int character;
	int curr_char = 0;
	int curr_string = 0;

	// ABRE EL ARCHIVO
	file = fopen(params->filename, "r");
	if (file == NULL){
		fprintf(stderr, "(read_lines): can't open file");
		exit(1);
	}

	// ALOJA MEMORIA PARA LOS STRINGS DEL HILO
	threads_strings[params->id] = (string*) malloc(sizeof(string)*strings_amount);
	if(threads_strings[params->id] == NULL){
		fprintf(stderr, "(read_lines) can't allocate in threads_strings\n");
		exit(1);
	}

	// ALOJA MEMORIA PARA EL PRIMER STRING
	threads_strings[params->id][curr_string] = (string) malloc(sizeof(char)*string_size);
	if(threads_strings[params->id][curr_string] == NULL){
		fprintf(stderr, "(read_lines) can't allocate in string\n");
		exit(1);
	}

	// LEE CARACTER POR CARACTER MIENTRAS NO SEA EL FINAL DEL ARCHIVO
	while((character = fgetc(file)) != EOF){
		// SI ES UN SALTO DE LINEA O ESPACIO, CONTINUAR
		if(isspace(character) != 0 && curr_char == 0){
			continue;
		} else if (isspace(character) == 0 && curr_char == 0) {
			number_of_lines++;
		}

		// SI LA CANTIDAD DE STRINGS ES MAYOR A LA ALOJADA, RE-ALOJA EN MEMORIA
		if(curr_string >= strings_amount - 1) {
			strings_amount += 5;
			threads_strings[params->id] = (string*) realloc(threads_strings[params->id], sizeof(string)*strings_amount);
			if(threads_strings[params->id] == NULL){
				fprintf(stderr, "(read_lines) can't reallocate in threads_strings\n");
				exit(1);
			}
		}

		// SI LA CANTIDAD DE CARACTERES ES MAYOR AL DEL STRING, RE-ALOJA EN MEMORIA
		if(curr_char >= string_size - 1) {
			string_size += 10;
			threads_strings[params->id][curr_string] = (string) realloc(threads_strings[params->id][curr_string],sizeof(char)*string_size);
			if(threads_strings[params->id][curr_string] == NULL){
				fprintf(stderr, "(read_lines) can't reallocate in buffer\n");
				exit(1);
			}
		}

		// SI EL CARACTER ES UN SALTO DE LINEA, AÑADIR EL CARACTER NULO Y PASAR EL SIGUIENTE STRING
		// SI NO, GUARDAR EL CARACTER EN EL STRING ACTUAL
		if(character == '\n'){
			threads_strings[params->id][curr_string][curr_char] = '\0';
			threads_strings[params->id][curr_string] = strdup(trim(threads_strings[params->id][curr_string]));
			curr_string++;
			curr_char = 0;
			string_size = 10;
			threads_strings[params->id][curr_string] = (string) malloc(sizeof(char)*string_size);
		} else {
			threads_strings[params->id][curr_string][curr_char] = character;
			curr_char++;
		}
	}

	threads_strings[params->id][curr_string][curr_char] = '\0';
	threads_strings[params->id][curr_string] = strdup(trim(threads_strings[params->id][curr_string]));

	thread_stats[params->id].number_of_lines = number_of_lines;
	fclose(file);
}

/* 3. worker_function: Esta función llama a todas las funciones que debe ejecutar
cada hilo trabajador, además de inicializar ciertos datos de cada hilo */

void *worker_function (void *arg) {
	params_t *params = (params_t*) arg;
	read_lines(params);
	store_lines(params);
	save_thread_data(params);
	int number_of_lines = thread_stats[params->id].number_of_lines;
	printf("This worker thread writes %d lines to \"%s.sorted\"\n", number_of_lines, params->filename);
	free_thread(params);
}

/* 2. init_worker_threads: Crea todos los hilos trabajadores, además, antes
de crearlos, asigna un id y un archivo a cada hilo creado en el arreglo thread_params */

void init_worker_threads (int argc, char *argv[]) {
	for (int i = 1; i < argc; i++) {
		thread_params[i - 1].filename = argv[i];
		thread_params[i - 1].id = i - 1;
		pthread_create(&worker_threads[i - 1], NULL, worker_function, thread_params + (i-1));
	}
}

/* 1. init_threads_data: Aloja la cantidad de memoria necesaria para las estructuras
de datos de los hilos trabajadores de la PARTE I y II*/

void init_threads_data (int argc) {
	threads_strings = (string**) malloc(sizeof(string*)*(argc-1));
	thread_params = (params_t*) malloc(sizeof(params_t)*(argc-1));
	thread_stats = (stats_t*) malloc(sizeof(stats_t)*(argc-1));
	worker_threads = (pthread_t*) malloc(sizeof(pthread_t)*(argc-1));
}

// 0. Main

int main(int argc, char *argv[]) {
	if(argc < 3){
		printf("Error: almost 2 values are needed as arguments\n");
		exit(1);
	}
	init_threads_data(argc);
	init_worker_threads(argc,argv);
	wait_for_workers(argc);
	open_sorted_files(argc,argv);
	init_merge();
	print_final_data(argc);
	//free_threads_data(argc);
	return 0;
}