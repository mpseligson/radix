/* Copywrite 2019 Matthew Seligson
 *
 * The benchmarkSort.c class is used to generate benchmark performance data for the serial radix sort algorithm
 * and C's stdlib quicksort algorithm. The user is free to change the input array by modifying the array builder
 * in main.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

/* Returns a random integer in range [min, max].
 */
int randomInRange(int min, int max) {
  return rand() % (max - min + 1) + min;
}

/* Returns the maximum integer in an array of length n.
 */
int max(int *input, int n) {
  int retval = 0;
  for (int i = 0; i < n; i++) {
    if (input[i] > retval) {
      retval = input[i];
    }
  }
  return retval;
}

/* Counting sort stable sorts the input by the digit in the exp'th place and modifies
 * count[] to reflect the histogram of exp'th place digit values.
 *
 * implementation inspired by geeksforgeeks.org/radix-sort/
 */
void countingSort(int *input, int n, int exp) {
  // output will be written to input at the end of the function
  int *output = malloc(n * sizeof(int));
  int count[10] = {0};
  int i;
  int index;

  // build count array
  for (i = 0; i < n; i++) {
    count[(input[i] / exp) % 10]++;
  }

  // modify count array to be cumulative sum array
  for (i = 1; i < 10; i++) {
    count[i] += count[i-1];
  }

  // place input values in final positions according to workingCount
  for (i = n - 1; i >= 0; i--) {
    index = (input[i] / exp) % 10;
    output[count[index] - 1] = input[i];
    count[index]--;
  }

  // assign output to original array
  for (i = 0; i < n; i++) {
    input[i] = output[i];
  }
  free(output);
}

// comparison function to pass to Qsort
// https://stackoverflow.com/questions/1787996/c-library-function-to-perform-sort
int comp (const void * elem1, const void * elem2) {
  int f = *((int*)elem1);
  int s = *((int*)elem2);
  if (f > s) return  1;
  if (f < s) return -1;
  return 0;
}

/* Standard serial radix sort
 */
void uniprocessorRadixSort(int *input, int n) {
  int m = max(input, n);
  for (int exp = 1; m / exp > 0; exp *= 10) {
    countingSort(input, n, exp);
  }
}

// run serial radix sort using predefined seeds for array generator
int main(int argc, const char * argv[]) {
  int seeds[] = {1, 2, 22443, 16882, 7931, 10723, 24902, 124, 25282, 2132};
  int runs = 10;
  clock_t start, end;
  double cpu_time_used;
  double total = 0;

  for (int size = 1; size < 20000000; size *= 2) {
    for (int run = 0; run < runs; run++) {
      srand(seeds[run]);
      int *input = malloc(size * sizeof(int));
      for (int i = 0; i < size; i++) {
	// to build random arrays use the line below
	input[i] = randomInRange(0, size);

	// to build ordered arrays use the line below
	// input[i] = i;

	// to use reverse ordered arrays use the line below
	// input[i] = size - i - 1;

	// to build random array with exactly one digit use the line below
	// input[i] = randomInRange(0, 9);
	
	// to build random array with exactly four digits use the line below
	//input[i] = randomInRange(1000, 9999);

	// to build random array with exactly eight digits use the line below
	//input[i] = randomInRange(10000000, 99999999);
      }
      start = clock();
      
      // to benchmark serial radix sort use the line below
      uniprocessorRadixSort(input, size);

      // to benchmark qsort use the line below
      // qsort(input, size, sizeof(int), comp);

      end = clock();
      cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
      total += cpu_time_used;
      free(input);
      if (run == runs - 1) printf("%d,%f\n", size, total / runs);
    }
  }
}
