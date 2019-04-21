/*****************************************************************************
 * CMSC 691 Homework 3
 * Author: Justin Rokisky (GK81776)
 * OpenMP Bucket Sort
 *
 * Overview:
 * To avoid interference between threads and to reduce memory waste, each
 * thread gets its own linked list per bucket.
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
typedef struct Bucket
{
	int value;
	struct Bucket *next;
} Bucket;

// Print the values in a bucket.
void print_bucket(const Bucket *bucket) 
{
	if (bucket->value == -1) {
		return;
	}
	printf(" %d ->", bucket->value);
	Bucket *iterator = bucket->next;
	while (iterator != NULL) {
		printf(" %d ->", iterator->value);
		iterator = iterator->next;
	}
	printf("\n");
}

// Append an element to the end of a bucket.
void append_bucket(Bucket *bucket, int value)
{
	if (bucket->value == -1) {
		bucket->value = value;
		return;
	}

	Bucket *iterator = bucket;
	while (iterator->next != NULL) {
		iterator = iterator->next;
	}

	Bucket *new_val = malloc(sizeof(Bucket));
	new_val->value = value;
	new_val->next = NULL;
	iterator->next = new_val;
}

int size(Bucket *bucket) 
{	
	if (bucket->value == -1) {
		return 0;
	}

	int count = 1;
	Bucket *iterator = bucket;
	while (iterator->next != NULL) {
		iterator = iterator->next;
		count++;
	}
	return count;
}

void copy(int *array_bucket, Bucket *bucket, int *curr_idx)
{
	if (bucket->value == -1) {
		return;
	}
	array_bucket[*curr_idx] = bucket->value;
	(*curr_idx)++;

	Bucket *iterator = bucket;
	while (iterator->next != NULL) {
		iterator = iterator->next;
		array_bucket[*curr_idx] = iterator->value;
		(*curr_idx)++;
	}
}

void insertion_sort(int *arr, int size)
{
	int i, j, val;
	for (i = 1; i < size; i++) {
		val = arr[i];
		j = i - 1;

		while (j >= 0 && arr[j] > val) {
			arr[j+1] = arr[j];
			j = j - 1;
		}
		arr[j+1] = val;
	}
}

int main (int argc, char *argv[]) {
	// Max value of data interval.
	int _interval_max = 100;		//INT_MAX;
	// Number of elements.
	int _num_keys = 2924;
	// TODO: this should be configurable as input.
	int _num_threads = 5;
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
	output(data, _num_keys);
	printf("\n");

	// Each thread gets its own collection of Buckets so we don't
	// have to worry about writes interfering.
	Bucket *all_buckets = malloc(_num_threads * _num_buckets * sizeof(Bucket));
	int **results = malloc(_num_buckets * sizeof(int *));
	int *results_bucket_sizes = malloc(_num_buckets * sizeof(int));

	// Init struct values.
	for (i = 0; i < _num_threads; i++) {
		for (j = 0; j < _num_buckets; j++) {
			all_buckets[i * _num_buckets + j].value = -1;
			all_buckets[i * _num_buckets + j].next = NULL;
		}
	}

	data_chunk_size = _num_keys / _num_threads;
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

		#pragma omp barrier

		// Each thread is responsible for the bucket that matches its tid. Loop through each
		// threads local version of that bucket and concatenate them all together.
		bucket_size = 0;
		for (i = 0; i < _num_threads; i++) {
			bucket_size += size(&all_buckets[i * _num_threads + tid]);
		}
		results[tid] = malloc(bucket_size * sizeof(int));
		results_bucket_sizes[tid] = bucket_size;
		int j = 0;
		for (i = 0; i < _num_threads; i++) {
			copy(results[tid], &all_buckets[i * _num_threads + tid], &j);
		}
		insertion_sort(results[tid], bucket_size);
	}

	// Print Results.
	for (i = 0; i < _num_buckets; i++) {
		output(results[i], results_bucket_sizes[i]);
	}
	printf("\n");

	return 0;
}


