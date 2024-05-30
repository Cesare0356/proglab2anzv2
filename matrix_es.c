#include "matrix.h"
void genera_matrice(char* filename) {
    char caratteri[] = "abcdeghijklmnopqrstuvwxyz";
    //Se il nome del file è vuoto, genero la matrice casualmente
    if(strcmp(filename,"") == 0) {
        for(int i=0; i<N; i++) {
            for(int j=0; j<N; j++)
                matrix[i][j] = toupper(caratteri[rand() % strlen(caratteri)]);
        }
    } else {
        //Apro il file della matrice
        FILE *file;
        file = fopen(filename,"r");
        if(file == NULL) {
            printf("Errore apertura\n");
            exit(EXIT_FAILURE);
        }
        char c;
        int i=0;
        int j=0;
        int linea = 0;
        
        //Avanzo fino alla linea corrente nel file
        while (linea <= curr_linea && (c = fgetc(file)) != EOF) {
            if (c == '\n')
                linea++;
        }

        //Se raggiungo la fine del file, ricomincio dall'inizio
        if(c == EOF) {
            fseek(file, 0, SEEK_SET);
            curr_linea = -1;
            linea = 0;
        }

        //Riempio la matrice leggendo il file
        while ((c = fgetc(file)) != EOF && i < N) {
            if (c == '\n')
                break;
            if (c == 'Q' || c == 'q') {  
                //Salto la 'U' successiva alla 'Q'
                matrix[i][j] = toupper(c);  
                j++;
                fgetc(file);  
            } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                matrix[i][j] = toupper(c);
                j++;
            } else
                continue;
            if (j == N) {
                j = 0;
                i++;
            }
        }

        //Aggiorno la linea corrente
        curr_linea = linea;
        fclose(file);
    }
}

//Converto matrice in stringa
void matrice_tostring(char* buffer) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (matrix[i][j] == 'Q')
                sprintf(buffer + strlen(buffer), "%c%c\t", matrix[i][j], 'u');
            else
                sprintf(buffer + strlen(buffer), "%c\t", matrix[i][j]);
        }
        strcat(buffer, "\n");
    }
}

void printList(Nodo* head) {
    if(head == NULL) {
        printf("NULL\n");
        return;
    }
    printf("%s ->",head->word);
    printList(head->next);
}

//Aggiungo parola nella lista
void aggiungi_parola(Nodo **head, char *parola) {
    Nodo *new_node = (Nodo*)malloc(sizeof(Nodo));
    if(new_node == NULL) {
        exit(EXIT_FAILURE);
    }
    strcpy(new_node->word, parola);
    new_node->next = *head;
    *head = new_node;
    printf("AGGIUNGO\n");
    printList(*head);
}

//Rimuovo parola dalla lista
void rimuovi_parola(Nodo **head){
    if (head == NULL) return;
    Nodo* temp = *head;
    *head = (*head)->next;
    free(temp);
    printf("RIMUOVO\n");
    printList(*head);
}

//Verifico se la parola è stata presa
int is_parola_presa(Nodo *head, char *parola) {
    Nodo *curr = head;
    while (curr != NULL) {
        if (strcmp(curr->word, parola) == 0)
            return 1;
        curr = curr->next;
    }
    return 0;
}

int is_valid(int x, int y, int visited[N][N]) {
    //Ritorno 1 se la posizione è all'interno della matrice e non è stata visitata
    return (x>=0 && x<N && y>=0 && y<N && !visited[x][y]);
}

int trova_parola(char matrix[N][N], int visited[N][N], char *parola, int index, int x, int y) {
    //Se l'indice è >= lunghezza parola, ho trovato la parola
    if(index >= (int)strlen(parola))
        return 1;

    //Se la posizione non è valida ritorno 0
    if(!is_valid(x,y,visited))
        return 0;
    
    //Direzioni per muoversi nella matrice
    int r[N];
    r[0] = -1; r[1] = 0; r[2] = 0; r[3] = 1;
    int c[N];
    c[0] = 0; c[1] = -1; c[2] = 1; c[3] = 0; 

    //Se la cella corrente contiene 'Q' e la parola contiene "Qu"
    if(matrix[x][y] == 'Q' && parola[index] == 'Q' && parola[index+1] == 'U') {
        //Segno la cella come visitata
        visited[x][y] = 1;
        //Provo a trovare parola nella direzione specificata
        for(int i=0; i<N; i++) {
            if(trova_parola(matrix,visited,parola,index+2,x+r[i],y+c[i]))
                return 1;
        }
        //Segno cella come non visitata
        visited[x][y] = 0;
    } //Se la cella corrente contiene il carattere atteso nella parola
    else if (matrix[x][y] == parola[index]) {
        //Stesso procedimento di sopra
        visited[x][y] = 1;
        for (int i = 0; i < N; i++) {
            if (trova_parola(matrix, visited, parola, index + 1, x + r[i], y + c[i]))
                return 1;
        }
        visited[x][y] = 0;
    }

    //La parola non è stata trovata
    return 0;
}

int is_parola_valida(char matrix[N][N], char *parola) {
    //Se la parola è più corta di 4 lettere non è valida
    if(strlen(parola) < 4)
        return 0;

    //Controllo che ogni 'Q' sia seguito da 'U'
    for(int i=0; i<(int)strlen(parola); i++) {
        if(parola[i] == 'Q')
            if(i+1 >= (int)strlen(parola) || parola[i+1] != 'U')
                return 0;
    }

    //Inizializzo matrice dei visitati a 0
    int visited[N][N];
    for (int i=0; i<N; i++) 
        for (int j=0; j<N; j++) 
            visited[i][j] = 0;
    
    //Cerco parola nella matrice partendo da ogni cella
    for(int i=0; i<N; i++) {
        for(int j=0; j<N; j++) {
            if(trova_parola(matrix,visited,parola,0,i,j)) {
                return 1;
            }
        }
    }

    //Non ho trovato la parola
    return 0;
}

//Ottengo indice di un carattere A-Z
int get_index(char c) {
    return c - 'A';
}

//Creo un nuovo nodo nel trie
TrieNodo* crea_tnodo(void) {
    TrieNodo *node = (TrieNodo *)malloc(sizeof(TrieNodo));
    if (node == NULL) {
        printf("Errore allocazione TNODO");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < MAXCHARS; i++) {
        node->children[i] = NULL;
    }
    node->terminal = 0;
    return node;
}

void inserisci_trie(TrieNodo *root, char *parola) {
    TrieNodo *node = root;
    while(*parola != '\0') {
        if(*parola >= 'A' && *parola <= 'Z') {
            //Calcolo indice del carattere
            int i = get_index(*parola);
            if(!node->children[i]) {
                //Creo un nuovo nodo se non esiste
                node->children[i] = crea_tnodo();
            }
            node = node->children[i];
        }
        //Salto la 'U' successiva alla 'Q'
        if(*parola == 'Q' && *(parola+1) == 'U')
            parola++;
        parola++;
    }
    //Imposto il flag terminale a 1
    node->terminal = 1;
}

int trova_trie(TrieNodo *node, char *parola) {
    //Ritorno il flag terminale se raggiungo la fine della parola
    if(*parola == '\0')
        return node->terminal;
    if(*parola >= 'A' && *parola <= 'Z') {
        int i = get_index(*parola);
        //Se il figlio non esiste ritorno 0
        if(node->children[i] == NULL)
            return 0;
        //Salto la 'U' successiva alla 'Q'
        if(*parola == 'Q' && *(parola+1)  == 'U')
            return trova_trie(node->children[i],parola+2);
        return trova_trie(node->children[i],parola+1);
    }
    //Parola non valida restituisco 0
    return 0;
}

void load_dic(TrieNodo *root, char *nomefile) {
    //Apro il dizionario
    FILE *file = fopen(nomefile,"r");
    if(file == NULL) {
        printf("Errore apertura dizionario\n");
        exit(EXIT_FAILURE);
    }
    char parola[256];
    //Leggo ogni parola del file e la inserisco nel trie
    while (fscanf(file,"%s",parola) != EOF) {
        //Converto parola in maiuscolo
        for(int i=0; parola[i]; i++)
            parola[i] = toupper(parola[i]);
        inserisci_trie(root,parola);
    }
    fclose(file);
}