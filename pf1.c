#include "pf1.h"

pthread_t *worker_threads;
params_t *thread_params;
stats_t *thread_stats;
sem_t mutex;
string **threads_strings;
FILE **available_files;
int max_available;

void free_threads_data (int argc) {
	free(threads_strings);
	free(thread_params);
	free(thread_stats);
	free(worker_threads);
}

void wait_for_workers (int argc) {
	for (int i = 0; i < argc - 1; i++) {
		pthread_join (worker_threads[i],NULL);
	}
}

void free_thread (params_t *params) {
	int number_of_lines = thread_stats[params->id].number_of_lines;
	for(int i = 0; i < number_of_lines; i++){
		free(threads_strings[params->id][i]);
	}
	free(threads_strings[params->id]);
}

void fill_merge_data(int argc, char* argv[]) {
	available_files = (FILE**) malloc(sizeof(FILE*)*argc-1);
	max_available = argc-1;
	string url;
	for(int i = 1; i < argc; i++){
		url = strdup(argv[i]);
		strcat(url,".sorted");
		available_files[i - 1] = fopen(url,"r");
	}
}

void merge_sorted_files(argc) {
	while(max_available != 1){
		if(max_available%2 == 0){
			printf("Par (%d), creando %d hilos\n", max_available, max_available/2);
			max_available = (max_available/2);
		} else {
			printf("Impar (%d), creando %d hilos\n", max_available, max_available/2);
			max_available = (max_available/2) + 1;
		}
	}
}

int compare_by_length (const void* p, const void* q) {
	string str_1 = *(const string*) p; 
    string str_2 = *(const string*) q;
	if(strlen(str_1) < strlen(str_2)) {
		return -1;
	}
	if(strlen(str_1) > strlen(str_2)) {
		return 1;
	}
	if(strlen(str_1) == strlen(str_2)){
		return 0;
	}
}

void save_thread_data (params_t *params) {
	int number_of_lines = thread_stats[params->id].number_of_lines;
	qsort((void*) threads_strings[params->id], number_of_lines, sizeof(string), compare_by_length);
	thread_stats[params->id].shortest_line = threads_strings[params->id][0];
	thread_stats[params->id].longest_line = threads_strings[params->id][number_of_lines - 1];
	if(number_of_lines == 0) {
		thread_stats[params->id].shortest_line = "\n";
		thread_stats[params->id].longest_line = "\n";
	}
}

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

void store_lines (params_t *params) {
	FILE *file;
	size_t n = 0;
	string sorted_filename = strdup(params->filename);
	int number_of_lines = thread_stats[params->id].number_of_lines;
	strcat(sorted_filename,".sorted");
	file = fopen(sorted_filename, "w");
	if(file == NULL){
		printf("ERROR FATAL (store_lines): No se pudo abrir '%s'\n",sorted_filename);
		pthread_exit(NULL);
	}
	qsort((void *) threads_strings[params->id], number_of_lines, sizeof(string), compare_by_alphabet);
	for(int i = 0; i < number_of_lines; i++){
		fprintf(file,"%s",threads_strings[params->id][i]);
	}
	fclose(file);
}

void load_lines (params_t *params) {
	FILE *file;
	size_t n = 0;
	char* line = NULL;
	file = fopen(params->filename, "r");
	if (file == NULL){
		printf("ERROR FATAL (load_lines): No se pudo abrir '%s'\n",params->filename);
		pthread_exit(NULL);
	}
	for(int i = 0; getline(&line,&n,file) != -1; i++){
		threads_strings[params->id][i] = strdup(line);
	}
	fclose(file);
	free(line);
}

int count_lines (char* filename) {
	FILE *file;
	size_t n = 0;
	char* line = NULL;
	int number_of_lines = 0;
	file = fopen(filename, "r");
	if (file == NULL){
		printf("ERROR FATAL (count_lines): No se pudo abrir '%s'\n",filename);
		pthread_exit(NULL);
	}
	while(getline(&line,&n,file) != -1){
		number_of_lines++;
	}
	fclose(file);
	free(line);
	return number_of_lines;
}

void *worker_function (void *arg) {
	params_t *params = (params_t*) arg;
	int number_of_lines = count_lines(params->filename);
	threads_strings[params->id] = malloc(sizeof(string)*number_of_lines);
	thread_stats[params->id].number_of_lines = number_of_lines;
	load_lines(params);
	store_lines(params);
	save_thread_data(params);
	printf("This worker thread writes %d lines to \"%s.sorted\"\nlongest: %sshortest: %s", number_of_lines, params->filename, thread_stats[params->id].longest_line, thread_stats[params->id].shortest_line);
	free_thread(params);
}

void init_worker_threads (int argc, char *argv[]) {
	for (int i = 1; i < argc; i++) {
		thread_params[i - 1].filename = argv[i];
		thread_params[i - 1].id = i - 1;
		pthread_create(&worker_threads[i - 1], NULL, worker_function, thread_params + (i-1));
	}
}

void init_threads_data (int argc) {
	threads_strings = (string**) malloc(sizeof(string*)*(argc-1));
	thread_params = (params_t*) malloc(sizeof(params_t)*(argc-1));
	thread_stats = (stats_t*) malloc(sizeof(stats_t)*(argc-1));
	worker_threads = (pthread_t*) malloc(sizeof(pthread_t)*(argc-1));
}

int main(int argc, char *argv[]) {
	init_threads_data(argc);
	init_worker_threads(argc,argv);
	wait_for_workers(argc);
	fill_merge_data(argc,argv);
	merge_sorted_files();
	free_threads_data(argc);
	return 0;
}