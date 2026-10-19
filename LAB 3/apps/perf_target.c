#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define ARRAY_SIZE 100000
#define ITERATIONS 50

/*
 * CPU-intensive workload.
 * sqrt() and log() are executed repeatedly.
 */
double heavy_compute(const double *arr, int n)
{
    double result = 0.0;

    for (int i = 0; i < n; i++) {
        result += sqrt(arr[i]) * log(arr[i] + 1.0);
    }

    return result;
}

/* Linear search: O(n) */
int linear_search(const int *arr, int n, int target)
{
    for (int i = 0; i < n; i++) {
        if (arr[i] == target)
            return i;
    }

    return -1;
}

/* File I/O workload */
int io_intensive(int count)
{
    FILE *f = fopen("/tmp/lab03_perf_test.txt", "w");

    if (f == NULL) {
        perror("fopen");
        return -1;
    }

    for (int i = 0; i < count; i++) {
        fprintf(f, "Line %d: measurement data\n", i);
    }

    if (fclose(f) != 0) {
        perror("fclose");
        return -1;
    }

    return 0;
}

int main(void)
{
    printf("=== LAB03 PERFORMANCE TARGET ===\n");

    double *darr = malloc(sizeof(double) * ARRAY_SIZE);
    int *iarr = malloc(sizeof(int) * ARRAY_SIZE);

    if (darr == NULL || iarr == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        free(darr);
        free(iarr);
        return 1;
    }

    for (int i = 0; i < ARRAY_SIZE; i++) {
        darr[i] = (double)(i + 1);
        iarr[i] = i;
    }

    printf("[1] CPU-intensive computation...\n");

    double total = 0.0;

    for (int iter = 0; iter < ITERATIONS; iter++) {
        total += heavy_compute(darr, ARRAY_SIZE);
    }

    printf("Compute result: %.4f\n", total);

    printf("[2] Linear search 1000 times...\n");

    long search_sum = 0;

    for (int i = 0; i < 1000; i++) {
        search_sum += linear_search(
            iarr, ARRAY_SIZE, ARRAY_SIZE - 1
        );
    }

    /* Use the result to prevent unused-work elimination. */
    printf("Search checksum: %ld\n", search_sum);

    printf("[3] Writing 10000 lines...\n");

    if (io_intensive(10000) != 0) {
        free(darr);
        free(iarr);
        return 1;
    }

    free(darr);
    free(iarr);

    printf("Done.\n");

    return 0;
}
