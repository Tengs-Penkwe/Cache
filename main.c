#include "cache.h"
#include <stdio.h>
#include <time.h>

#define MAX_SIZE 1024

typedef int64_t array_t[MAX_SIZE * MAX_SIZE];
static cache_t *cache;

/* Array used for testing. The aligned attribute is used to ensure
 * that test_array[0] is stored at the start of a cache block, for any
 * block size up to 1024.
 */
array_t __attribute__ ((aligned (1024))) test_array;

/*
 * Write data to the array, bypassing the "cache".
 */
void fillArray(array_t a) {
  
    size_t i, j;
  
  for (i = 0; i < MAX_SIZE; i++)
    for (j = 0; j < MAX_SIZE; j++)
      a[i * MAX_SIZE + j] = (i + 1) * (j + 1);
}

int64_t sumA(array_t a, size_t rows, size_t cols) {
  
    size_t i, j;
    int64_t sum = 0;

    for (i = 0; i < rows; i++)
	    for (j = 0; j < cols; j++)
	        sum += cache_read(cache, (uintptr_t) &a[i * cols + j], rand);
  
    return sum;
}

int64_t sumB(array_t a, size_t rows, size_t cols) {
  
    size_t i, j;
    int64_t sum = 0;

    for (j = 0; j < cols; j++)
	    for (i = 0; i < rows; i++)
	        sum += cache_read(cache, (uintptr_t) &a[i * cols + j], rand);
  
    return sum;
}

int64_t sumC(array_t a, size_t rows, size_t cols) {
  
    size_t i, j;
    int64_t sum = 0;
  
    for (j = 0; j < cols; j += 2)
    	for (i = 0; i < rows; i += 2)
	        sum += cache_read(cache, (uintptr_t) &a[i * cols + j], rand) +
		    cache_read(cache, (uintptr_t) &a[(i+1) * cols + j], rand) +
		    cache_read(cache, (uintptr_t) &a[i * cols + j+1], rand) +
		    cache_read(cache, (uintptr_t) &a[(i+1) * cols + j+1], rand);
  
    return sum;
}

int64_t sumD(array_t a, size_t rows, size_t cols, size_t count) {

    size_t i, j, k;
    int64_t sum = 0;

    for (k = 0; k < count; k++)
        for (i = 0; i < rows; i++)
            for (j = 0; j < cols; j += 8)
                sum += cache_read(cache, (uintptr_t) &a[i * cols + j + k], rand);

    return sum;
}

void print_stats() {
  
    size_t mc = cache_miss_count(cache);
    size_t ac = cache_access_count(cache);
  
    if (ac == 0) {
	    printf("The cache wasn't used.\n");
    }
    else {
	    printf("Miss rate = %8.4f\n", (double) mc/ac);
    }
}

int main() {

    srand(time(NULL));
  
    fillArray(test_array);
  
    cache = cache_new(16384, 64, 1, CACHE_REPLACEMENTPOLICY_LRU);
    printf("Sum = %lld\n", sumA(test_array, 64, 64));
    print_stats();
    cache_free(cache);
  
    cache = cache_new(16384, 64, 1, CACHE_REPLACEMENTPOLICY_LRU);
    printf("Sum = %lld\n", sumB(test_array, 64, 64));
    print_stats();
    cache_free(cache);
  
    cache = cache_new(16384, 64, 1, CACHE_REPLACEMENTPOLICY_LRU);
    printf("Sum = %lld\n", sumC(test_array, 64, 64));
    print_stats();
    cache_free(cache);

    cache = cache_new(16384, 64, 4, CACHE_REPLACEMENTPOLICY_LRU);
    printf("Sum = %lld\n", sumA(test_array, 64, 64));
    print_stats();
    cache_free(cache);
  
    cache = cache_new(16384, 64, 4, CACHE_REPLACEMENTPOLICY_LRU);
    printf("Sum = %lld\n", sumB(test_array, 64, 64));
    print_stats();
    cache_free(cache);
  
    cache = cache_new(16384, 64, 4, CACHE_REPLACEMENTPOLICY_LRU);
    printf("Sum = %lld\n", sumC(test_array, 64, 64));
    print_stats();
    cache_free(cache);

    cache = cache_new(16384, 64, 8, CACHE_REPLACEMENTPOLICY_RANDOMIZED_MARKING);
    printf("Sum = %lld\n", sumD(test_array, 12, 256, 5));
    print_stats();
    cache_free(cache);

    cache = cache_new(16384, 64, 8, CACHE_REPLACEMENTPOLICY_RANDOMIZED_MARKING);
    printf("Sum = %lld\n", sumD(test_array, 12, 256, 5));
    print_stats();
    cache_free(cache);

    cache = cache_new(16384, 64, 8, CACHE_REPLACEMENTPOLICY_RANDOMIZED_MARKING);
    printf("Sum = %lld\n", sumD(test_array, 12, 256, 5));
    print_stats();
    cache_free(cache);

    return 0;
}

