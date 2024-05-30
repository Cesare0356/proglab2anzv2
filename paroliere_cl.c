#include "includes.h"
#include "dataclient.h"

//Variabili globali utilizzate
int client_socket;
pthread_t send_thread, receive_thread;
int registrato = 0;

//Funzione per rendere in maiuscolo una stringa
void to_upper(char *str) {
    while (*str != '\0') {
        *str = toupper(*str);
        str++;
    }
}

void handle_sigint() {
    printf("Chiusura client\n");
    message_info msg;
    msg.type = MSG_CHIUSURA_CLIENT;
    msg.length = 0;
    strcpy(msg.data, "");
    //Invio un messaggio di chiusura al server
    send_message(client_socket, &msg);

    //Chiudo il socket
    close(client_socket);
   
    //Cancello i thread di invio e ricezione
    pthread_cancel(send_thread);
    pthread_cancel(receive_thread);
   
    //Termino il processo
    exit(0);
}

void* send_messages(void* arg) {
    free(arg);
    char command[256];
    char buffer[256];
    message_info msg;
    ssize_t e;
    printf("[PROMPT PAROLIERE]-->\n");
    while (1) {
        //Leggo un comando dall'input standard
        SYSC(e, read(STDIN_FILENO, command, 256), "nella read");

        //Gestione dei vari comandi disponibili
        if (strcmp(command, "aiuto\n") == 0) {
            printf("Comandi disponibili:\n");
            printf("  aiuto\n");
            printf("  registra_utente <nome utente>\n");
            printf("  matrice\n");
            printf("  p <parola>\n");
            printf("  fine\n");
            printf("  classifica\n");
            printf("[PROMPT PAROLIERE]-->\n");
        } else if (!registrato && sscanf(command, "r %10s", buffer) == 1) {
            //Comando per registrare un utente
            msg.type = MSG_REGISTRA_UTENTE;
            strcpy(msg.data, buffer);
            msg.length = strlen(msg.data);
            send_message(client_socket, &msg);
            msg.type = MSG_MATRICE;
            send_message(client_socket, &msg);
        } else if (registrato) {
            if (strcmp(command, "matrice\n") == 0) {
                //Comando per richiedere la matrice
                msg.type = MSG_MATRICE;
                msg.length = 0;
                send_message(client_socket, &msg);
            } else if (registrato && sscanf(command, "p %16s", buffer) == 1) {
                //Comando per inviare una parola
                msg.type = MSG_PAROLA;
                to_upper(buffer);
                strcpy(msg.data, buffer);
                msg.length = strlen(buffer);
                send_message(client_socket, &msg);
            } else if (strcmp(command, "fine\n") == 0) {
                //Comando per chiudere il client
                msg.type = MSG_CHIUSURA_CLIENT;
                msg.length = 0;
                send_message(client_socket, &msg);
                break;
            } else if (strcmp(command, "classifica\n") == 0) {
                //Comando per richiedere la classifica
                msg.type = MSG_CLASSIFICA;
                msg.length = 0;
                send_message(client_socket, &msg);
            } else {
                printf("Digita 'aiuto'\n");
                printf("[PROMPT PAROLIERE]-->\n");
            }
        } else if (strcmp(command, "fine\n") == 0) {
            //Comando per chiudere il client se non registrato
            msg.type = MSG_CHIUSURA_CLIENT;
            msg.length = 0;
            send_message(client_socket, &msg);
            break;
        } else {
            printf("Registrati per fare comandi\n");
            printf("[PROMPT PAROLIERE]-->\n");
        }
        //Pulisco il buffer dei comandi
        memset(command, 0, sizeof(command));
    }
    pthread_exit(NULL);
}

void* receive_messages(void* arg) {
    free(arg);
    message_info msg;
    while (1) {
        //Ricevo un messaggio dal server
        receive_message(client_socket, &msg);

        //Gestisco i vari tipi di messaggi ricevuti
        if (msg.type == MSG_OK) {
            printf("Utente registrato!\n");
            registrato = 1;
        } else if (msg.type == MSG_ERR) {
            printf("%s\n", msg.data);
            printf("[PROMPT PAROLIERE]-->\n");
        } else if (msg.type == MSG_MATRICE) {
            printf("%s\n", msg.data);
        } else if (msg.type == MSG_TEMPO_PARTITA) {
            printf("%s\n", msg.data);
            printf("[PROMPT PAROLIERE]-->\n");
        } else if (msg.type == MSG_TEMPO_ATTESA) {
            printf("%s\n", msg.data);
            printf("[PROMPT PAROLIERE]-->\n");
        } else if (msg.type == MSG_PUNTI_PAROLA) {
            if (atoi(msg.data) == 0)
                printf("Parola già inserita\n");
            else
                printf("Punti ottenuti da questa parola: %s\n", msg.data);
            printf("[PROMPT PAROLIERE]-->\n");
        } else if (msg.type == MSG_CHIUSURA_CLIENT) {
            printf("Chiusura client!\n");
            //Chiudo il socket e cancello i thread
            close(client_socket);
            pthread_cancel(send_thread);
            pthread_cancel(receive_thread);
            pthread_exit(NULL);
        } else if (msg.type == MSG_PUNTI_FINALI) {
            printf("Classifica finale:\n");
            printf("%s\n", msg.data);
            printf("[PROMPT PAROLIERE]-->\n");
        } else if (msg.type == MSG_CLASSIFICA) {
            printf("%s\n", msg.data);
            printf("[PROMPT PAROLIERE]-->\n");
        }
    }
    pthread_exit(NULL);
}

void client_fun(char* nome_server, int porta_server) {
    int retvalue;
    struct sockaddr_in server_addr;

    //Creo il socket del client
    SYSC(client_socket, socket(AF_INET, SOCK_STREAM, 0), "nella socket");

    //Imposto i parametri della socket del server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(porta_server);
    server_addr.sin_addr.s_addr = inet_addr(nome_server);

    //Effettuo la connessione al server
    SYSC(retvalue, connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)), "nella connect");

    //Imposto il gestore del segnale SIGINT
    signal(SIGINT, handle_sigint);

    //Creo i thread per inviare e ricevere messaggi
    pthread_create(&send_thread, NULL, send_messages, NULL);
    pthread_create(&receive_thread, NULL, receive_messages, NULL);

    //Attendo la terminazione dei thread
    pthread_join(send_thread, NULL);
    pthread_join(receive_thread, NULL);
    return;
}

int main(int argc, char* argv[]) {
    //Controllo che il numero di argomenti sia sufficiente
    if (argc < 3) {
        fprintf(stderr, "Uso: nome_server porta_server\n");
        exit(EXIT_FAILURE);
    }
    
    //Ottengo il nome del server e la porta dal comando
    char* nome_server = argv[1];
    int porta_server = atoi(argv[2]);

    //Avvio la funzione principale del client
    client_fun(nome_server, porta_server);
}

