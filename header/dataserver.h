#include <pthread.h>
#define MAX_CLIENTS 10
//Enum per gli stati della partita
typedef enum {
    STATO_ATTESA,
    STATO_INGIOCO,
    STATO_PAUSA
} game_state;
game_state curr_state = STATO_ATTESA;

//Mutex per sincronizzazione
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//Variabili globali utilizzate
unsigned int seed = 0;
int server_socket;
int active_clients = 0;
char *dizionario = "dictionary_ita.txt";
char *data_filename="";
int tempo_attesa = 7;  
int durata = 15;
time_t inizio_partita;
time_t fine_partita;
char classifica[256];

//Funzioni da implementare
void *thread_client(void *arg);
void server(char *nome_server, int porta);
void handle_sigint();
void handle_alarm();
int tempo_rim();
void *thread_scorer(void *arg);
