#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
//Nodo scorer
typedef struct score_t {
    char username[11];
    int score;
    struct score_t* next;
} score_t;

//Coda scorer
typedef struct {
    score_t* head;
    score_t* tail;
    pthread_mutex_t mutex_score;
    pthread_cond_t cond_score;
} score_queue;

//Funzioni da implementare
void init_queue(score_queue* queue);
void enqueue_score(score_queue* queue, char* username, int score);
score_t* deque_score(score_queue* queue);
int compare_scores_final(const void* a, const void* b);