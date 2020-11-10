#include "encrypt.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

FILE *input_file;
FILE *output_file;
int input_counts[256];
int output_counts[256];
int key = 1;

void clear_counts() {
	memset(input_counts, 0, sizeof(input_counts));
	memset(output_counts, 0, sizeof(output_counts));
}

void *random_reset() {
	while (1) {
		sleep(rand() % 11 + 5);
		reset_requested();
		key = rand() % 26;
		clear_counts();
		reset_finished();
	}
}

void init() {
	pthread_t pid;
	srand(time(0));
	pthread_create(&pid, NULL, &random_reset, NULL);
}

void open_input(char *name) {
	init();
	input_file = fopen(name, "r");
}

void open_output(char *name) {
	output_file = fopen(name, "w");
}

int read_input() {
	usleep(10000);
	return fgetc(input_file);
}

void write_output(int c) {
	fputc(c, output_file);
}

int caesar_encrypt(int c) {
	if (c >= 'a' && c <= 'z') {
		c += key;
		if (c > 'z') {
			c = c - 'z' + 'a' - 1;
		}
	} else if (c >= 'A' && c <= 'Z') {
		c += key;
		if (c > 'Z') {
			c = c - 'Z' + 'A' - 1;
		}
	}
	return c;
}

void count_input(int c) {
	input_counts[toupper(c)]++;
}

void count_output(int c) {
	output_counts[toupper(c)]++;
}

int get_input_count(int c) {
	return input_counts[c];
}

int get_output_count(int c) {
	return output_counts[c];
}

