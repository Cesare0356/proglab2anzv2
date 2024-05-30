#include "includes.h"


void receive_message(int socket, message_info* msg){
    //Ricevo messaggi tramite syscalls
    int retvalue;

    //Nota: ho messo gli errori di chiusura client in modo che se un utente non è registrato ma il server si chiude viene disconnesso correttamente
    if ((retvalue = read(socket, &msg->length, sizeof(unsigned int))) <= 0) {
        msg->type = MSG_CHIUSURA_CLIENT; 
        return;
    }
    if ((retvalue = read(socket, &msg->type, sizeof(char))) <= 0) {
        msg->type = MSG_CHIUSURA_CLIENT;
        return;
    }
    if ((retvalue = read(socket, msg->data, sizeof(msg->data))) <= 0) {
        msg->type = MSG_CHIUSURA_CLIENT;
        return;
    }

    //Stringa null-terminated
    msg->data[sizeof(msg->data) - 1] = '\0';
}
void send_message(int socket,message_info* msg){
    int retvalue;
    //Invio il messaggio tramite syscalls
    SYSC(retvalue,write(socket,&msg->length,sizeof(unsigned int)),"nella scrittura della lunghezza del messaggio");
    SYSC(retvalue,write(socket,&msg->type,sizeof(char)),"nella scrittura del tipo di messaggio");
    SYSC(retvalue,write(socket,msg->data,sizeof(msg->data)),"nella scrittura del payload");
}