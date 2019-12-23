# radix
Parallelized Radix Sort w/ MPI

# Setup
To compile all of the programs, run 'make'.

Three of the produced executables are MPI programs. These are 'radix', 'radixBlocks', 'radixOBlocks' which correspond
to the R0, R1 and R2 implementations of parallel radix sort. To run one of these using MPI, use the following command:

'mpirun -np <number_of_processors> radix'

where 'radix' can be replaced with 'radixBlocks' or 'radixOBlocks' to run either of the other programs.

To run benchmarkSort, simply use the following command:

'./benchmarkSort'

By default, each of these programs run the sorting algorithm on random inputs for different sized inputs. The output of
each program in stdout will be formatted as "array size", "average sort time", and will print these pairs for array sizes
1, 2, 4... 2^24.

# R0, R1 and R2
R0 (radix.c) sends elements between processors one at a time using MPI_Isend() and MPI_Recv().

R1 (radixBlocks.c) sends elements between processors more efficiently than R0; all elements that need to be sent between processors A and B are packed together into an array of size N/P where N is the total number of elements and P is the number of processors.

R2 (radixOBlocks.c) sends elements between processors more efficiently than R1; all elements that need to be sent between processors A and B are packed together into an array of exactly the right size <= N/P where the size is first communicated between processors, then the array of exactly the right size can be received because its size is known by the destination processor.