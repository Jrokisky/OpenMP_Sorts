#!/bin/bash
gcc -o parallel-sample-sort parallel-sample-sort.c -fopenmp
gcc -o parallel-bucket-sort parallel-bucket-sort.c -fopenmp
rm results.txt

for SORT_NAME in ./parallel-bucket-sort ./parallel-sample-sort
do
	echo $SORT_NAME >> results.txt
	for NUM_KEYS in 10000000 100000000 200000000

	do
		for NUM_THREAD in 1 2 4 8 16 
		do
			$SORT_NAME $NUM_THREAD $NUM_KEYS 0 >> results.txt
			$SORT_NAME $NUM_THREAD $NUM_KEYS 0 >> results.txt
			echo >> results.txt 
		done
		echo "------------------------------------------------------" >> results.txt
	done
	echo "=================================================================" >> results.txt
	echo >> results.txt
done
