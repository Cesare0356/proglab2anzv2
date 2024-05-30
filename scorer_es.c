#include "scorer.h"

//Inizionalizzo la coda
void init_queue(score_queue* queue) {
    queue->head = NULL;
    queue->tail = NULL;
    pthread_mutex_init(&queue->mutex_score,NULL);
    pthread_cond_init(&queue->cond_score,NULL);
}

void enqueue_score(score_queue* queue, char* username, int score) {
    //Alloco memoria per punteggio
    score_t* s = malloc(sizeof(score_t));
    if(s == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }    
    strcpy(s->username,username);
    s->score = score;
    s->next = NULL;

    //Aggiungo punteggio alla coda
    pthread_mutex_lock(&queue->mutex_score);
    if(queue->tail)
        queue->tail->next = s;
    else
        queue->head = s;
    queue->tail = s;
    pthread_cond_signal(&queue->cond_score);
    pthread_mutex_unlock(&queue->mutex_score); 
}

score_t* deque_score(score_queue* queue) {
    pthread_mutex_lock(&queue->mutex_score);

    //Aspetto che ci siano punteggi nella coda
    while(queue->head == NULL)
        pthread_cond_wait(&queue->cond_score,&queue->mutex_score);
    
    //Estraggo punteggio dalla coda
    score_t* s = queue->head;
    queue->head = s->next;
    if(queue->head == NULL)
        queue->tail = NULL;
    pthread_mutex_unlock(&queue->mutex_score);
    return s;
}

//Funzione per comparare i risultati nella qsort dello scorer
int compare_scores_final(const void* a, const void* b) {
    score_t* scoreA = *(score_t**)a;
    score_t* scoreB = *(score_t**)b;
    return scoreB->score - scoreA->score;
}
