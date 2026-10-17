#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/*
 * LAB03 - Part A
 * Three intentional bugs for GDB remote debugging.
 *
 * BUG 1: Array out-of-bounds
 * BUG 2: Use-after-free
 * BUG 3: Signed integer overflow
 */

/* BUG 1: Loop executes one iteration too many. */
void process_data(int *data, int len)
{
    int sum = 0;

    for (int i = 0; i <= len; i++) {
        /*
         * BUG: Valid indices are 0 to len-1.
         * When i == len, data[i] is out-of-bounds.
         */
        sum += data[i];
    }

    printf("Sum = %d\n", sum);
}

/* BUG 2: Return a pointer to freed memory. */
char *create_message(const char *text)
{
    char *buf = malloc(strlen(text) + 1);

    if (buf == NULL)
        return NULL;

    strcpy(buf, text);

    free(buf);

    /*
     * BUG: buf no longer points to valid
     * allocated memory after free().
     */
    return buf;
}

/* BUG 3: Multiplication exceeds signed int range. */
int calculate(int a, int b)
{
    /*
     * BUG: 100000 * 100000 exceeds INT_MAX.
     * Signed integer overflow is undefined behavior.
     */
    int result = a * b;

    printf("calculate(%d, %d) = %d\n",
           a, b, result);

    return result;
}

int main(void)
{
    printf("=== LAB03 BUGGY APP ===\n");
    fflush(stdout);

    int arr[5] = {10, 20, 30, 40, 50};

    printf("[TEST1] Array processing\n");
    process_data(arr, 5);

    printf("[TEST2] Use-after-free\n");

    char *msg = create_message("Hello Embedded Linux");

    /*
     * BUG: msg may point to freed memory.
     * This may cause invalid output or a crash.
     */
    if (msg != NULL)
        printf("Message: %s\n", msg);

    printf("[TEST3] Integer overflow\n");
    calculate(100000, 100000);

    printf("Done.\n");

    return 0;
}
