/*****************************************************************************
 * CMSC 691 Homework 3
 * Author: Justin Rokisky (GK81776)
 * OpenMP Bucket Sort
 *
 * To Build:
 * 	gcc -o parallel-bucket-sort parallel-bucket-sort.c -fopenmp
 *
 * To Run:
 * 	parallel-bucket-sort NUM_THREADS NUM_KEYS DISPLAY_OUTPUT[1|0]
 *
 * Overview:
 * 	Bucket sort using OpenMP.
 *
 * Design:
 * 	* To avoid interference between threads and to reduce memory waste,
 * 	  the data structure used is a linked list for each bucket for each 
 * 	  thread.
 * 	* Each Bucket stores its size and tail for fast appending.
 * 
 *****************************************************************************/

#include<stdlib.h>
#include<stdio.h>
#include<limits.h>
#include<time.h>
#include<omp.h>

// Print array.
void output (int *array, int n) {
	int i;
	for (i=0; i < n; i++) {
		printf("%d ", array[i]);
	}
}

// Linked list.
typedef struct LinkedList
{
	int value;
	struct LinkedList *next;
} LinkedList;

typedef struct Bucket
{
	int count;
	struct LinkedList *head;
	struct LinkedList *tail;
} Bucket;

// Print the values in a bucket.
void print_bucket(const Bucket *bucket) 
{
	if (bucket->head == NULL) {
		return;
	}
	printf(" %d ->", bucket->head->value);
	LinkedList *iterator = bucket->head->next;
	while (iterator != NULL) {
		printf(" %d ->", iterator->value);
		iterator = iterator->next;
	}
	printf("\n");
}

// Append an element to the end of a bucket.
void append_bucket(Bucket *bucket, int value)
{
	LinkedList *new_val = malloc(sizeof(LinkedList));
	new_val->value = value;
	new_val->next = NULL;

	// Base Case.
	if (bucket->head == NULL) {
		bucket->head = new_val;
		bucket->tail = new_val;
		bucket->count = 1;
		return;
	}

	LinkedList *tail = bucket->tail;
	tail->next = new_val;
	bucket->tail = new_val;
	(bucket->count)++;
}

// Copy the given bucket into the given array starting at the given index.
void copy(int *array_bucket, Bucket *bucket, int *curr_idx)
{
	if (bucket->head == NULL) {
		return;
	}
	array_bucket[*curr_idx] = bucket->head->value;
	(*curr_idx)++;

	LinkedList *iterator = bucket->head;
	while (iterator->next != NULL) {
		iterator = iterator->next;
		array_bucket[*curr_idx] = iterator->value;
		(*curr_idx)++;
	}
}

int compare (const void * a, const void * b) {
   return ( *(int*)a - *(int*)b );
}


int main (int argc, char *argv[]) {

	if (argc != 4) {
		printf("Usage: parallel-bucket-sort NUM_THREADS NUM_KEYS OUTPUT\n");
		return 1;
	}

	int _num_threads = atoi(argv[1]);
	int _num_keys = atoi(argv[2]);
	int _output = atoi(argv[3]);

	double start = omp_get_wtime();
	// Max value of data interval.
	int _interval_max = INT_MAX;
	int _num_buckets = _num_threads;
	int i, j, tid, data_start_idx, data_end_idx;
	int data_chunk_size, bucket_range, bucket_idx, bucket_size;

	// Generate data.
	int *data;
	data = malloc(_num_keys*sizeof(int));
	srand(time(NULL));
	for (i = 0; i < _num_keys; i++) {
		data[i] = random() % _interval_max;
	}
	if (_output == 1) {
		output(data, _num_keys);
		printf("\n");
	}

	// Each thread gets its own collection of Buckets to avoid slowdowns
	// due to locking.
	Bucket *all_buckets = malloc(_num_threads * _num_buckets * sizeof(Bucket));
	int **results = malloc(_num_buckets * sizeof(int *));
	int *results_bucket_sizes = malloc(_num_buckets * sizeof(int));

	// Init struct values.
	for (i = 0; i < _num_threads; i++) {
		for (j = 0; j < _num_buckets; j++) {
			all_buckets[i * _num_buckets + j].count = 0;
			all_buckets[i * _num_buckets + j].head = NULL;
			all_buckets[i * _num_buckets + j].tail = NULL;
		}
	}

	// The size of data that each thread is responsible for bucket-ing.
	data_chunk_size = _num_keys / _num_threads;
	// The range of each bucket.
	bucket_range = _interval_max / _num_buckets;

	#pragma omp parallel private(tid, i, j, data_start_idx, data_end_idx, bucket_idx, bucket_size) num_threads(_num_threads)
	{
		tid = omp_get_thread_num();
		
		// Get the start and end indexes of the data this thread will analyze.
		data_start_idx = tid * data_chunk_size;
		data_end_idx = data_start_idx + data_chunk_size;
		if (tid == (_num_threads - 1)) {
			data_end_idx = _num_keys;
		}

		// Insert data into this threads private buckets.
		for (i = data_start_idx; i < data_end_idx; i++) {
			bucket_idx = data[i] / bucket_range;
			// Last bucket case.
			if (bucket_idx > (_num_buckets - 1)) {
				bucket_idx = _num_buckets - 1;
			}
			append_bucket(&all_buckets[tid * _num_threads + bucket_idx], data[i]);
		}

		// Wait here to ensure that all threads have bucket-ed all the data 
		// they're responsible for.
		#pragma omp barrier

		// Each thread is responsible for sorting the bucket that matches its tid. 
		// Loop through each threads local version of that bucket and concatenate 
		// its data with the rest.
		bucket_size = 0;
		for (i = 0; i < _num_threads; i++) {
			bucket_size += all_buckets[i * _num_threads + tid].count;
		}

		// Allocate space for the sorted result.
		results[tid] = malloc(bucket_size * sizeof(int));
		results_bucket_sizes[tid] = bucket_size;

		// Copy each threads' bucket data into an array for sorting.
		int j = 0;
		for (i = 0; i < _num_threads; i++) {
			copy(results[tid], &all_buckets[i * _num_threads + tid], &j);
		}
		
		// Sort the bucket data.
		qsort(results[tid], bucket_size, sizeof(int), compare);
	}

	double end = omp_get_wtime() - start;

	// Print Results.
	for (i = 0; i < _num_buckets; i++) {
		if (_output == 1) {
			output(results[i], results_bucket_sizes[i]);
		}
	}
	printf("%f seconds taken using %d threads to sort %d ints\n", end, _num_threads, _num_keys);

	return 0;
}

