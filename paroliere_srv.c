#include "header/dataserver.h"
#include "header/matrix.h"
#include "header/includes.h"
#include "header/scorer.h"
//Struttura per contenere i dati del client
typedef struct {
    int socket;
    char username[11];
    int score;
    Nodo* lis; //Lista delle parole inviate dal client
} client_data;
//Array per memorizzare i dati di tutti i client
client_data clients[MAX_CLIENTS];

int main(int argc, char *argv[]) {
    //Controllo che il numero di argomenti sia sufficiente
    if (argc < 3) {
        fprintf(stderr, "Uso: nome_server porta_server\n");
        exit(EXIT_FAILURE);
    }
    
    //Ottengo il nome del server e la porta dal comando
    char *nome_server = argv[1];
    int porta_server = atoi(argv[2]);

    //Definisco le opzioni lunghe per il parsing degli argomenti
    struct option long_options[] = {
        {"matrici", required_argument, 0, 'm'},
        {"durata", required_argument, 0, 'd'},
        {"seed", required_argument, 0, 's'},
        {"diz", required_argument, 0, 'z'},
        {0, 0, 0, 0}
    };
    
    int opt, option_index = 0;

    //Faccio il parsing degli argomenti della riga di comando
    while ((opt = getopt_long(argc, argv, "m:d:s:z:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'm':
                //Assegno il nome del file delle matrici
                data_filename = optarg;
                struct stat file_stat;

                //Controllo se il file delle matrici può essere aperto
                if (stat(data_filename, &file_stat) != 0) {
                    fprintf(stderr, "Errore: Il file delle matrici non può essere aperto.\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'd':
                //Imposto la durata della partita in secondi
                durata = atoi(optarg) * 60;
                break;
            case 's':
                //Imposto il seed per la generazione casuale
                seed = (unsigned int)atoi(optarg);
                break;
            case 'z':
                //Assegno il nome del dizionario
                dizionario = optarg;
                break;
            default:
                //Se getopt_long ritorna '?' vuol dire che c'è stata un'opzione non riconosciuta
                exit(EXIT_FAILURE);
        }
    }

    //Verifico che non siano utilizzati contemporaneamente il file delle matrici e il seed
    if(strlen(data_filename) > 0 && seed != 0) {
        fprintf(stderr, "Non puoi usare data_filename e seed.\n");
        exit(EXIT_FAILURE);
    }

    //Inizializzo il seed per la generazione casuale
    if (seed)
        srand(seed);
    else
        srand(time(NULL));

    //Avvio il server
    server(nome_server, porta_server);
    return 0;
}

void server(char *nome_server, int porta_server) {
    struct sockaddr_in server_addr, client_addr;
    int client_socket, retvalue;
    socklen_t addr_size = sizeof(client_addr);

    //Creo la socket del server
    SYSC(server_socket, socket(AF_INET, SOCK_STREAM, 0), "nella socket");

    //Imposto i parametri della socket del server
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(nome_server);
    server_addr.sin_port = htons(porta_server);

    //Effettuo il bind della socket del server
    SYSC(retvalue, bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)), "nella bind");

    //Metto la socket del server in ascolto
    SYSC(retvalue, listen(server_socket, MAX_CLIENTS), "nella listen");
    printf("Connessione server stabilita\n");

    //Imposto i gestori dei segnali SIGINT e SIGALRM
    signal(SIGINT, handle_sigint);
    signal(SIGALRM, handle_alarm);

    //Genero la matrice iniziale
    genera_matrice(data_filename);

    //Entro nel ciclo principale del server per accettare connessioni dei client
    while (1) {
        SYSC(client_socket, accept(server_socket, (struct sockaddr *)&client_addr, &addr_size), "nella accept");
        int *c = malloc(sizeof(int));
        if (c == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        *c = client_socket;

        //Creo un nuovo thread per gestire il client
        pthread_t thread;
        pthread_create(&thread, NULL, thread_client, c);
        
        //Marco il thread come distaccato
        pthread_detach(thread);
    }

    //Chiudo la socket del server
    SYSC(retvalue, close(server_socket), "nella close");
}

void handle_sigint() {
    int retvalue;
    message_info msg;

    //Invio un messaggio di chiusura a tutti i client attivi
    msg.type = MSG_CHIUSURA_CLIENT;
    for (int i = 0; i < active_clients; i++) {
        send_message(clients[i].socket, &msg);
    }

    //Chiudo la socket del server
    SYSC(retvalue, close(server_socket), "nella close ao");
    exit(0);
}

void* thread_client(void* arg) {
    int client_socket = *((int*)arg);
    int registrato = 0;
    message_info msg;
    client_data *client = NULL;

    //Creo il nodo radice del trie
    TrieNodo *dic = crea_tnodo();
    int retv;

    //Carico il dizionario nel trie
    load_dic(dic, dizionario);

    while (1) {
        receive_message(client_socket, &msg);

        //Gestisco la registrazione dell'utente
        if (msg.type == MSG_REGISTRA_UTENTE) {
            int exist = 0;

            //Verifico se l'utente è già registrato
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < active_clients; i++) {
                if (strcmp(clients[i].username, msg.data) == 0) {
                    exist = 1;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);

            //Registro l'utente se non è già registrato
            if (!exist) {
                client = &clients[active_clients];
                client->socket = client_socket;
                strcpy(client->username, msg.data);
                client->score = 0;
                client->lis = NULL;
                active_clients++;
                msg.type = MSG_OK;
                printf("Utente registrato: %s\n", client->username);
                registrato = 1;

                //Se il gioco è in attesa, cambio stato a in gioco
                if (curr_state == STATO_ATTESA) {
                    curr_state = STATO_INGIOCO;
                    inizio_partita = time(NULL);
                    alarm(durata);
                }
            } else {
                //Invio un messaggio di errore se l'utente è già registrato
                msg.type = MSG_ERR;
                sprintf(msg.data, "Utente già registrato");
                msg.length = strlen(msg.data);
            }
            send_message(client_socket, &msg);
        } else if (msg.type == MSG_MATRICE && registrato) {
            //Invio la matrice al client
            char buffer[256];

            //Calcolo il tempo rimanente
            int tempo_rimanente = tempo_rim();

            //Se la partita è in gioco invio matrice e tempo rimanente
            if (curr_state == STATO_INGIOCO) {
                matrice_tostring(buffer);
                
                //Invio la matrice al client
                msg.type = MSG_MATRICE;
                strcpy(msg.data, buffer);
                msg.length = strlen(msg.data);
                send_message(client_socket, &msg);

                //Invio il tempo rimanente al client
                msg.type = MSG_TEMPO_PARTITA;
                sprintf(buffer, "Tempo rimanente:%d secondi", tempo_rimanente);
                strcpy(msg.data, buffer);
                msg.length = strlen(msg.data);
                send_message(client_socket, &msg);
            } else if (curr_state == STATO_PAUSA) {
                //Invio il tempo di attesa al client se la partita è in pausa
                msg.type = MSG_TEMPO_ATTESA;
                sprintf(buffer, "Tempo rimanente inizio nuova partita:%d secondi", tempo_rimanente);
                strcpy(msg.data, buffer);
                msg.length = strlen(msg.data);
                send_message(client_socket, &msg);
            }
            //Pulisco il buffer
            memset(buffer, 0, sizeof(buffer));
        } else if (msg.type == MSG_PAROLA && curr_state != STATO_INGIOCO) {
            //Gestisco l'errore quando il gioco è in pausa e si invia una parola
            msg.type = MSG_ERR;
            sprintf(msg.data, "Gioco in pausa, non puoi inserire parole");
            msg.length = strlen(msg.data);
            send_message(client_socket, &msg);
        } else if (msg.type == MSG_PAROLA && curr_state == STATO_INGIOCO) {
            //Gestisco l'inserimento di una parola valida
            char buffer[16];
            int punteggio = 0;
            msg.type = MSG_PUNTI_PAROLA;

            //Controllo se la parola è già stata presa dal client
            if (is_parola_presa(client->lis, msg.data)) {
                client->score += punteggio;
            } //Controllo se la parola è valida nella matrice 
            else if (is_parola_valida(matrix, msg.data)) {
                //Controllo se la parola è nel dizionario
                if (trova_trie(dic, msg.data)) {
                    aggiungi_parola(&client->lis, msg.data);
                    for (int i = 0; i < (int)strlen(msg.data); i++)
                        punteggio += 1;
                    client->score += punteggio;
                } else {
                    //Invio un messaggio di errore se la parola non è nel dizionario
                    msg.type = MSG_ERR;
                    sprintf(msg.data, "Parola non nel dizionario");
                    msg.length = strlen(msg.data);
                    send_message(client_socket, &msg);
                    continue;
                }
            } else {
                //Invio un messaggio di errore se la parola non è valida
                msg.type = MSG_ERR;
                sprintf(msg.data, "Parola non corretta");
                msg.length = strlen(msg.data);
                send_message(client_socket, &msg);
                continue;
            }

            //Aggiorno il punteggio del client
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < active_clients; i++) {
                if (clients[i].socket == client_socket) {
                    clients[i] = *client;
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);
            
            //Invio il punteggio ottenuto al client
            sprintf(buffer, "%d", punteggio);
            strcpy(msg.data, buffer);
            msg.length = strlen(msg.data);
            send_message(client_socket, &msg);
            memset(buffer, 0, sizeof(buffer));
        } else if(msg.type == MSG_CLASSIFICA) {
            //Se la partita è in pausa, il client può vedere la classifica
            if(curr_state == STATO_PAUSA) {
                strcpy(msg.data, classifica);
                msg.length = strlen(msg.data);
                send_message(client_socket, &msg);
            } //Se la partita è in corso, il client non può vedere la classifica
            else if (curr_state == STATO_INGIOCO) {
                sprintf(msg.data,"Partita ancora in corso");
                msg.length = strlen(msg.data);
                send_message(client_socket, &msg);
            }
        } else if (msg.type == MSG_CHIUSURA_CLIENT) {
            //Gestisco la chiusura del client
            printf("Chiudo il client\n");
            SYSC(retv, close(client_socket), "nella close");
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < active_clients; i++) {
                if (clients[i].socket == client_socket) {
                    while(clients[i].lis != NULL) {
                        rimuovi_parola(&(clients[i]).lis);
                    }        
                    free(clients[i].lis);
                    if (clients[i].socket == client_socket) {
                        clients[i] = clients[--active_clients];
                    }
                    break;
                }
            }
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL);
        }
    }
    pthread_exit(NULL);
}

void handle_alarm() {
    if (curr_state == STATO_INGIOCO) {
        curr_state = STATO_PAUSA;
        fine_partita = time(NULL);

        pthread_t scorer;

        // Creo la coda per i risultati
        score_queue *q = malloc(sizeof(score_queue));
        if (q == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        init_queue(q);

        for (int i = 0; i < active_clients; i++) {
            enqueue_score(q, clients[i].username, clients[i].score);
        }
        //pthread_cond_signal(&q->cond_score);

        pthread_create(&scorer, NULL, thread_scorer, q);
        pthread_detach(scorer);

        // Azzero lo score dei clients e le parole prese nella partita precedente
        pthread_mutex_lock(&mutex);
        for (int i = 0; i < active_clients; i++) {
            clients[i].score = 0;
            while (clients[i].lis != NULL) {
                printf("Lunghezza:%d\n",(int)strlen(clients[i].lis->word));
                //printf("Rimuovo parola per client %s: %s\n", clients[i].username, clients[i].lis->word);
                rimuovi_parola(&(clients[i]).lis);
            }
        }
        pthread_mutex_unlock(&mutex);
        printf("Sono esploso?\n");

        // Imposto il tempo di pausa della partita
        alarm(tempo_attesa);
    } else if (curr_state == STATO_PAUSA) {
        curr_state = STATO_INGIOCO;
        message_info msg;
        char buffer[256];
        genera_matrice(data_filename);

        // Segno il tempo di inizio partita
        inizio_partita = time(NULL);

        // Invio la nuova matrice ai clients e la durata della partita
        msg.type = MSG_MATRICE;
        strcpy(buffer, "\n\n\nNuova partita!\n");
        matrice_tostring(buffer);
        msg.length = strlen(msg.data);
        strcpy(msg.data, buffer);

        for (int i = 0; i < active_clients; i++) {
            send_message(clients[i].socket, &msg);
        }

        int tempo_rimanente = tempo_rim();
        msg.type = MSG_TEMPO_PARTITA;
        sprintf(buffer, "Tempo rimanente:%d secondi", tempo_rimanente);
        strcpy(msg.data, buffer);
        msg.length = strlen(msg.data);

        for (int i = 0; i < active_clients; i++) {
            send_message(clients[i].socket, &msg);
        }

        memset(classifica, 0, sizeof(classifica));
        alarm(durata);
    }
}

void *thread_scorer(void *arg) {
    score_queue *q = (score_queue *)arg;
    message_info  msg;
     
    score_t* scores[MAX_CLIENTS];
    int count = 0;

    //Elaboro la coda dei punteggi
    while (q->head != NULL) {
        scores[count] = deque_score(q);
        count++;
    }

    //Ordino i punteggi in ordine decrescente
    qsort(scores, count, sizeof(score_t*), compare_scores_final);
    char buffer[1024] = "";
    
    //Genero la classifica in formato CSV
    strcat(buffer, "Username,Score,Note\n");
    for (int i = 0; i < count; i++) {
        strcat(buffer, scores[i]->username); 
        strcat(buffer, ",");
        char score_str[16];
        sprintf(score_str, "%d", scores[i]->score); 
        strcat(buffer, score_str); 
        if (i == 0) {
            strcat(buffer, ",Vincitore"); 
        }
        strcat(buffer, "\n");
    }

    //Invio la classifica a tutti i client
    msg.type = MSG_PUNTI_FINALI;
    strcpy(msg.data, buffer);
    msg.length = strlen(msg.data);
    strcpy(classifica, msg.data);
    
    for (int i = 0; i < active_clients; i++) {
        send_message(clients[i].socket, &msg);
    }

    //Libero memoria allocata
    for (int i = 0; i < count; i++) {
        free(scores[i]);
    }
    free(q);
    pthread_exit(NULL);
}

int tempo_rim() {
    //Prendo l'ora corrente
    time_t ora = time(NULL);
    if (curr_state == STATO_INGIOCO) {
        //Calcolo il tempo passato dall'inizio della partita
        int tempo_passato = difftime(ora, inizio_partita);

        //Calcolo il tempo rimanente sottraendo il tempo passato alla durata totale
        int tempo_rimanente = durata - tempo_passato;

        //Restituisco il tempo rimanente
        return tempo_rimanente > 0 ? tempo_rimanente : 0;
    } else {
        //Calcolo il tempo rimanente dalla fine della partita più il tempo di attesa
        int tempo_rimanente = difftime(fine_partita + tempo_attesa, ora);
        
        //Restituisco il tempo rimanente
        return tempo_rimanente > 0 ? tempo_rimanente : 0;
    }
}

