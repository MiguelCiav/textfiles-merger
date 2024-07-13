#include "pf1.h"

pthread_t *worker_threads;
string **threads_strings;
params_t *thread_params;
stats_t *thread_stats;

void free_threads_data (int argc) {
	free(threads_strings);
	free(thread_params);
	free(thread_stats);
	free(worker_threads);
}

void wait_for_workers (int argc) {
	for (int i = 0; i < argc - 1; i++) {
		pthread_join (worker_threads[i],NULL);
		printf("Hilo %d finalizado\n", i);
	}
}

void free_thread (params_t *params, int number_of_lines){
	for(int i = 0; i < number_of_lines; i++){
		free(threads_strings[params->id][i]);
	}
	free(threads_strings[params->id]);
}

void store_lines (params_t *params, int number_of_lines) {
	FILE *file;
	size_t n = 0;
	string sorted_filename = strdup(params->filename);
	strcat(sorted_filename,".sorted");
	file = fopen(sorted_filename, "w");
	if(file == NULL){
		printf("ERROR FATAL (store_lines): No se pudo abrir '%s'\n",sorted_filename);
		pthread_exit(NULL);
	}
	for(int i = 0; i < number_of_lines; i++){
		printf("Escribiendo: %s", threads_strings[params->id][i]);
		fprintf(file,"%s",threads_strings[params->id][i]);
	}
	fclose(file);
}

void load_lines (params_t *params){
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

int count_lines (char* filename){
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
	load_lines(params);
	store_lines(params,number_of_lines);
	free_thread(params,number_of_lines);
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
	worker_threads = (pthread_t*) malloc(sizeof(pthread_t) * (argc - 1));
}

int main(int argc, char *argv[]) {
	init_threads_data(argc);
	init_worker_threads(argc,argv);
	wait_for_workers(argc);
	free_threads_data(argc);
	return 0;
}