#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_RECORDS 100
#define MSG_SIZE 256

typedef struct {
    int id;
    char *name;
    int *scores;
} Record;

/* Tao record va cap phat bo nho tren heap */
Record *create_record(int id, const char *name, int num_scores)
{
    Record *r = malloc(sizeof(Record));

    r->id = id;

    /* Cap phat bo nho cho chuoi name */
    r->name = malloc(strlen(name) + 1);
    strcpy(r->name, name);

    /* Cap phat mang scores */
    r->scores = malloc(sizeof(int) * num_scores);

    for (int i = 0; i < num_scores; i++) {
        r->scores[i] = id * 10 + i;
    }

    return r;
}

/* BUG 1: Quen free name va scores */
void delete_record(Record *r)
{
    free(r->name);
    free(r->scores);

    free(r);
}

/* BUG 2: Quen free buffer trong vong lap */
void process_messages(int count)
{
    for (int i = 0; i < count; i++) {

        char *buf = malloc(MSG_SIZE);

        snprintf(
            buf,
            MSG_SIZE,
            "Message #%d: data payload here",
            i
        );

        if (i % 10 == 0) {
            printf("Processed: %s\n", buf);
        }

        free(buf);
    }
}

int main(void)
{
    printf("=== LAB03 MEMORY LEAK TEST ===\n");

    Record **records =
        malloc(sizeof(Record *) * NUM_RECORDS);

    printf("[1] Creating records...\n");

    for (int i = 0; i < NUM_RECORDS; i++) {
        records[i] = create_record(
            i,
            "EmbeddedLinuxStudent",
            5
        );
    }

    printf("[2] Deleting records...\n");

    for (int i = 0; i < NUM_RECORDS; i++) {
        delete_record(records[i]);
    }

    free(records);

    printf("[3] Processing messages...\n");

    process_messages(50);

    printf("Program finished.\n");

    return 0;
}
