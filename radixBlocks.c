/* Copywrite Matthew Seligson 2019
 *
 * The radixBlocks.c class implements R1, the second  iteration on the parallelized radix sorting algorithm.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <mpi.h>
#include <string.h>

/* Returns a random integer in range [min, max].
 */
int randomInRange(int min, int max) {
  return rand() % (max - min + 1) + min;
}

/* Returns the maximum integer in an array of length n.
 */
int max(int * input, int n) {
  int retval = 0;
  for (int i = 0; i < n; i++) {
    if (input[i] > retval) {
      retval = input[i];
    }
  }
  return retval;
}

/* Helper function that rints a string offset by tabs to distinguish between stdout 
 * between threads.
 */
void logger(char * str, int thread_id) {
  int mult = 3;
  char out[strlen(str) + thread_id * mult + 1];
  strcpy(out + thread_id * mult, str);
  for (int i = 0; i < thread_id; i++) {
    for (int j = 0; j < mult; j++) {
      out[mult * i + j] = '\t';
    }
  }
  out[strlen(out)] = '\0';
  printf("%s\n", out);
}

/* Counting sort stable sorts the input by the digit in the exp'th place and modifies
 * count[] to reflect the histogram of exp'th place digit values.
 *
 * implementation inspired by geeksforgeeks.org/radix-sort/
 */
void countingSort(int * input, int * count, int n, int exp) {
  // output will be written to input at the end of the function
  int * output = malloc(n * sizeof(int));
  // workingCount stores the count of values with each digit value, then is modified
  // to store the sum of previous counts
  int workingCount[10];
  int i;
  int index;

  // build count array
  for (i = 0; i < n; i++) {
    count[(input[i] / exp) % 10]++;
  }

  // copy count array to workingCount array
  for (i = 0; i < 10; i++) {
    workingCount[i] = count[i];
  }

  // modify workingCount so each value is the sum of the previous values
  for (i = 1; i < 10; i++) {
    workingCount[i] += workingCount[i - 1];
  }

  // place input values in final positions according to workingCount
  for (i = n - 1; i >= 0; i--) {
    index = (input[i] / exp) % 10;
    output[workingCount[index] - 1] = input[i];
    workingCount[index]--;
  }

  // assign output to original array
  for (i = 0; i < n; i++) {
    input[i] = output[i];
  }
  free(output);
}

/* Helper function to compute equality of two arrays of the same length n.
 * Used to check correctness of parallel sorting algorithm.
 */
int compareArrays(int * arrA, int * arrB, int n) {
  for (int i = 0; i < n; i++) {
    if (arrA[i] != arrB[i]) {
      return 0;
    }
  }
  return 1;
}

/* Helper function to sort input of length n using radix sort with counting sort subroutine.
 */
void uniprocessorRadixSort(int * input, int n) {
  int m = max(input, n);
  for (int exp = 1; m / exp > 0; exp *= 10) {
    int count[10] = {
      0
    };
    countingSort(input, count, n, exp);
  }
}

/* Helper function to check the correctness of parallel radix sort. Original input is copied
 * rather than destroyed.
 */
int checkCorrect(int * originalInput, int * sortedInput, int n) {
  int * copy = (int * ) malloc(n * sizeof(int));
  for (int i = 0; i < n; i++) {
    copy[i] = originalInput[i];
  }
  uniprocessorRadixSort(copy, n);
  int correct = compareArrays(copy, sortedInput, n);
  free(copy);
  return correct;
}

int main(int argc,
  const char * argv[]) {
  MPI_Init(NULL, NULL);
  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, & world_rank);
  int world_size;
  MPI_Comm_size(MPI_COMM_WORLD, & world_size);

  // seeds for random number generator to be used
  int seeds[] = {
    1,
    2,
    22443,
    16882,
    7931,
    10723,
    24902,
    124,
    25282,
    2132
  };

  // number of runs on which to average performance times for a particular array size
  int runs = 10;

  // variables for timing events
  clock_t start, end;
  double cpu_time_used;
  double total;

  // run on array sizes 1, 2, 4 ... 2^24
  // before a given run or a given size, use a barrier to ensure all processors start at the same time
  for (int size = 1; size < 20000000; size *= 2) {
    MPI_Barrier(MPI_COMM_WORLD);
    for (int run = 0; run < runs; run++) {
      MPI_Barrier(MPI_COMM_WORLD);

      int * input; // aggregate array pointer
      int * adjustedInput; // adjusted aggregate array pointer to account for artificially added zeros
      int n; // size of the array
      int adjustedN; // size of the array + tailOffset, the size of the adjusted array
      int m; // max value in the array
      int subinputSize; // size of the local array on each processor
      int tailOffset; // number of artificially added zeros

      if (world_rank == 0) {
        // seed the random number generator based on the run
        srand(seeds[run]);

        // generate input array with values between 0 and n
        n = size;
        input = (int * ) malloc(n * sizeof(int));
        for (int j = 0; j < n; j++) {
          // to generate random arrays use the line below
          input[j] = randomInRange(0, n);

          // to generate ordered arrays use the line below
          // input[j] = j;

          // to generate reverse ordered arrays use the line below
          // input[j] = n - j - 1;

          // to build random array with exactly one digit use the line below
          // input[j] = randomInRange(0, 9);

          // to build random array with exactly four digits use the line below
          // input[j] = randomInRange(1000, 9999);

          // to build random array with exactly eight digits use the line below
          // input[j] = randomInRange(10000000, 99999999);
        }

        // array created, start clock
        start = clock();

        // compute subinputSize; size of local array on each processor
        subinputSize = ceil(n / ((double) world_size));

        // compute tailOffset; number of zeros to add to array to make sure each 
        // processor has exactly n/p elements
        tailOffset = world_size * subinputSize - n;

        // rebuild array with adjusted size to account for tailOffset zeros
        adjustedN = subinputSize * world_size;
        adjustedInput = (int * ) malloc(adjustedN * sizeof(int));
        for (int i = 0; i < adjustedN; i++) {
          if (i < tailOffset) {
            adjustedInput[i] = 0;
          } else {
            adjustedInput[i] = input[i - tailOffset];
          }
        }

        // zero out the total time variable before first run
        if (run == 0) {
          total = 0;
        }

        // find maximum value element
        m = max(input, n);
      }

      // broadcast values needed by all processors but computed on P0 to all processors
      MPI_Bcast( & m, 1, MPI_INT, 0, MPI_COMM_WORLD);
      MPI_Bcast( & n, 1, MPI_INT, 0, MPI_COMM_WORLD);
      MPI_Bcast( & subinputSize, 1, MPI_INT, 0, MPI_COMM_WORLD);
      MPI_Bcast( & adjustedN, 1, MPI_INT, 0, MPI_COMM_WORLD);

      MPI_Barrier(MPI_COMM_WORLD);

      // allocate local array to hold n/p values
      int * subinput = (int * ) malloc(subinputSize * sizeof(int));

      // scatter input from P0 to all processors such that each processor holds n/p values in its respective subinput
      MPI_Scatter(adjustedInput, subinputSize, MPI_INTEGER, subinput, subinputSize, MPI_INTEGER, 0, MPI_COMM_WORLD);

      // allCounts will hold the histograms for all p processors after MPI_Allgather is called
      int allCounts[10 * world_size];

      // allocate local array to hold n/p values after redistribution step
      int * newSubinput = malloc(subinputSize * sizeof(int));

      // allocate 2D arrays to hold pointers to blocks to be sent to each processor and to be received from each processor
      int ** sendBlocks = malloc(world_size * sizeof(int * ));
      int ** receiveBlocks = malloc(world_size * sizeof(int * ));

      // allocate the blocks
      for (int i = 0; i < world_size; i++) {
        sendBlocks[i] = (int * ) malloc(subinputSize * 2 * sizeof(int));
        receiveBlocks[i] = (int * ) malloc(subinputSize * 2 * sizeof(int));
      }

      // run counting sort and redistribution for each each digit place
      for (int exp = 1; m / exp > 0; exp *= 10) {

        // set all values in the blocks to -1
        for (int i = 0; i < world_size; i++) {
          memset(sendBlocks[i], -1, subinputSize * 2 * sizeof(int));
        }

        int count[10] = {
          0
        }; // histogram for subinput array
        int allCountsSum[10] = {
          0
        }; // histogram for aggregate array
        int allCountsPrefixSum[10] = {
          0
        }; // cumulative sum array to be built from allCountsSum
        int allCountsSumLeft[10] = {
          0
        }; // histogram for elements "left" of current processor (P0, P1, Pcurrent)

        // run counting sort, save local histogram to count and share histogram with all other processors
        countingSort(subinput, count, subinputSize, exp);
        MPI_Allgather(count, 10, MPI_INTEGER, allCounts, 10, MPI_INTEGER, MPI_COMM_WORLD);

        // sum up histograms into aggregate histogram allCountsSum
        for (int i = 0; i < 10 * world_size; i++) {
          int lsd = i % 10;
          int p = i / 10;
          int val = allCounts[i];

          // add histogram values to allCountsSumLeft for all processors "left" of current processor
          if (p < world_rank) {
            allCountsSumLeft[lsd] += val;
          }
          allCountsSum[lsd] += val;
          allCountsPrefixSum[lsd] += val;
        }

        // build cumulative sum array
        for (int i = 1; i < 10; i++) {
          allCountsPrefixSum[i] += allCountsPrefixSum[i - 1];
        }

        // request and status variables to be passed to MPI send and receive functions
        MPI_Request request;
        MPI_Status status;

        // count of elements sent from current processor for each key/digit
        int lsdSent[10] = {
          0
        };

        int val, lsd, destIndex, destProcess, localDestIndex;

        // keep track of number of elements in each block
        int blockSent[10] = {
          0
        };

        // build blocks from values
        for (int i = 0; i < subinputSize; i++) {
          val = subinput[i];
          lsd = (subinput[i] / exp) % 10;

          destIndex = allCountsPrefixSum[lsd] - allCountsSum[lsd] + allCountsSumLeft[lsd] + lsdSent[lsd];
          lsdSent[lsd]++;
          destProcess = destIndex / subinputSize;
          localDestIndex = destIndex % subinputSize;

          // set value, local index in destination process
          sendBlocks[destProcess][blockSent[destProcess] * 2] = val;
          sendBlocks[destProcess][blockSent[destProcess] * 2 + 1] = localDestIndex;
          blockSent[destProcess]++;
        }

        // send and receive blocks
        for (int i = 0; i < world_size; i++) {
          MPI_Isend(sendBlocks[i], subinputSize * 2, MPI_INT, i, 0, MPI_COMM_WORLD, & request);
          MPI_Recv(receiveBlocks[i], subinputSize * 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, & status);
        }

        MPI_Barrier(MPI_COMM_WORLD);

        // build newSubinput from sent blocks
        for (int sender = 0; sender < world_size; sender++) {
          for (int i = 0; i < subinputSize; i++) {
            val = receiveBlocks[sender][2 * i];
            localDestIndex = receiveBlocks[sender][2 * i + 1];
            // if a -1 is read, stop reading from the block
            if (val == -1) {
              break;
            }
            newSubinput[localDestIndex] = val;
          }
        }

        // set the values in subinput to the values in newSubinput
        for (int i = 0; i < subinputSize; i++) {
          subinput[i] = newSubinput[i];
        }

      }

      // allocate output array to write aggregate sorted array to
      int * output;
      if (world_rank == 0) {
        output = (int * ) malloc(adjustedN * sizeof(int));
      }

      // gather all subinputs to aggregate array output
      MPI_Gather(subinput, subinputSize, MPI_INTEGER, & output[world_rank * subinputSize], subinputSize, MPI_INTEGER, 0, MPI_COMM_WORLD);

      if (world_rank == 0) {
        // all artificially added zeros are ignored by incrementing the output by the number of such zeros
        output += tailOffset;

        // all work by the algorithm is done, so end the clock
        end = clock();
        cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
        total += cpu_time_used;

        // check correctness of algorithm and return 1 if incorrect
        if (!checkCorrect(input, output, n)) {
          return 1;
        }

        // before freeing output, decrement its pointer to its original location
        output -= tailOffset;
        free(output);
        free(adjustedInput);

        // if this is the final run for a given array size, average the runs and print it out
        if (run == runs - 1) {
          printf("%d %f\n", size, total / runs);
        }
        free(input);
      }
      MPI_Barrier(MPI_COMM_WORLD);

      // free all blocks
      for (int i = 0; i < world_size; i++) {
        free(sendBlocks[i]);
        free(receiveBlocks[i]);
      }

      free(sendBlocks);
      free(receiveBlocks);
      free(subinput);
      free(newSubinput);
    }
  }
  MPI_Finalize();
}
