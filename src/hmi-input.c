#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include "../include/service-functions.h"

#define OUTPUT_MAX_LEN 11

unsigned short int acceptable_input ( char * );
void throttle_failed_handler(int);

// La funzione main esegue le operazioni relative al componente
// di input, noto come human-machine-interface_input, abbreviato hmi-input
int main() {

	signal(SIGUSR1, throttle_failed_handler);

	// Connessione del file descriptor del pipe per la comunicazione tra central
  // ECU e hmi-input. Il protocollo impone che il pipe sia creato
  // durante la fase di inizializzazione del processo central-ECU e che venga
  // aperto in sola scrittura dal processo hmi-input il quale vi
  // scriva non appena riceva da tastiera attraverso il terminale di input.
	int pipe_fd;
	while((pipe_fd = openat(AT_FDCWD, "tmp/hmi-in.pipe", O_WRONLY | O_NONBLOCK)) < 0){
		perror("hmi-input: openat pipe");
		sleep(1);
	}
	perror("hmi-input: CONNECTED");
	// Inizializzazione della stringa di input che rappresenta il comando
	// inserito nel terminale da parte dell'esecutore del programma.
	// Il contenuto viene trasmesso alla central-ECU.
	char * term_input;
	if((term_input = malloc(OUTPUT_MAX_LEN)) == NULL){
		perror("hmi-input: malloc");
		exit(EXIT_FAILURE);
	}

	printf("TERMINALE DI INPUT\n\n"
				 "Inserire una delle seguenti parole e premere invio:\n"
				 "INIZIO\n"
				 "PARCHEGGIO\n"
				 "ARRESTO\n");

	// Il ciclo successivo rappresenta il cuore del processo.
	// Il processo si mette in attesa di una stringa da parte
	// dello stdin (scanf) e una volta ottenuta la controlla per
	//rilevare se corrisponde ad un comando accettabile per il sistema e,
	// in caso di compatibilita, lo trasmette alla central-ECU,
	// altrimenti si mette in attesa di un nuovo comando.
	// L'inserimento del comando e` stato reso case insensitive
	while(true){
		if((scanf("%s", term_input)) < 0){
			perror("hmi-input: scanf");
			exit(EXIT_FAILURE);
		}
		getchar();
		unsigned short int input_flag = acceptable_input(term_input);
		printf("%d\n", input_flag);
		if(input_flag < 4){
			if(write(pipe_fd, &input_flag, sizeof(short int)) < 0){
				perror("hmi-input: write");
				exit(EXIT_FAILURE);
			}
		} else {
			printf("Digitazione del comando errata, inserire una "
						 "delle seguenti parole e premere invio:\n"
						 "INIZIO\n"
						 "PARCHEGGIO\n"
						 "ARRESTO\n");
		}
	}
}

// Funzione adibita al controllo del comando di input.
// Le stringhe sono rese interamente maiuscole e successivamente
// confrontate con i messaggi accettabili.
// I comandi accettati sono AVVIO, PARCHEGGIO e ARRESTO.
// Non e` stata utilizzata la funzione strcasecmp perché il programma
// favorisse una maggiore portabilità.
unsigned short int acceptable_input (char * input){
	char * upper = str_toupper(input);
	if(strcmp(upper, "INIZIO") == 0)
		return INIZIO;
	else if (strcmp(upper, "ARRESTO") == 0)
		return ARRESTO;
	else if (strcmp(upper, "PARCHEGGIO") == 0)
		return PARCHEGGIO;
	else
		return 4;
}

void throttle_failed_handler (int sig){
	write(STDOUT_FILENO, "\n", 1);
	exit(EXIT_SUCCESS);
}
