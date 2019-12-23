all : radix radixBlocks radixOBlocks benchmarkSort

radix : radix.c
	mpicc -o radix radix.c -lm

radixBlocks : radixBlocks.c
	mpicc -o radixBlocks radixBlocks.c -lm

radixOBlocks : radixOBlocks.c
	mpicc -o radixOBlocks radixOBlocks.c -lm

benchmarkSort : benchmarkSort.c
	gcc -o benchmarkSort benchmarkSort.c

clean:
	-rm $(objects) radix radixBlocks radixOBlocks benchmarkSort
