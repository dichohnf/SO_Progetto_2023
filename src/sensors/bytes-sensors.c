#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/random.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <signal.h>
#include "../../include/service-functions.h"

// MACROS
// INPUT_LEN: numero fisso di byte da leggere dall'input
#define INPUT_LEN 8

void signal_stp_handler( int );

	int log_fd;
	int comm_fd;

// Funzione main che opera le operazioni per le componenti forward-facing-radar,
// surround-view-cameras e per il ciclo principale di park-assist.
// Gli argomenti richiesti sono il nome della della modalità di esecuzione
// e il nome della componente da simulare.
// Metodo di utilizzo:
// ./bytes-sensors "NORMALE"/"ARTIFICIALE" "RADAR"/"CAMERAS"
int main(int argc, char * argv[]){

	// Verifica della corretta chiamata del processo
	if(argc != 3){
		perror("chiamata");
		exit(EXIT_FAILURE);
	}

	signal(SIGTSTP, signal_stp_handler);

	// Inizializzazione del file descriptor tramite apertura del file
	// da cui ottenere i bytes di input.
	// Il file aperto dipende dalla modalità di esecuzione scelta.
	int input_fd;
	if(!strcmp(argv[1], NORMALE))
		input_fd = open("/dev/urandom", O_RDONLY);
	else if (!strcmp(argv[1], ARTIFICIALE))
		input_fd = open("../../urandomARTIFICIALE.binary", O_RDONLY);
	else{
		perror("chiamata modalita:");
		exit(EXIT_FAILURE);
	}

  // Connessione del file descriptor del log file. Apertura in sola scrittura.
  // Qualora il file non esista viene creato. Qualora il file sia presente
  // si mantengono le precedenti scritture.
	if(!strcmp(argv[2], RADAR)){
		if(unlink("../../log/radar.log") < 0){
			perror("unlink");
			exit(EXIT_FAILURE);
		}
		if((log_fd = open("../../log/radar.log", O_WRONLY | O_APPEND | O_CREAT, 0644)) < 0){
			perror("open log");
			exit(EXIT_FAILURE);
		}
		if((comm_fd = open("../../tmp/radar.pipe", O_WRONLY)) < 0){
			perror("open pipe");
			exit(EXIT_FAILURE);
		}
	} else if (!strcmp(argv[2], CAMERAS)){
		if(unlink("../../log/cameras.log") < 0){
			perror("unlink");
			exit(EXIT_FAILURE);
		}
		if((log_fd = open("../../log/cameras.log", O_WRONLY | O_APPEND | O_CREAT, 0644)) < 0){
			perror("open log");
			exit(EXIT_FAILURE);
		}
		if((comm_fd = open("../../tmp/cameras.pipe", O_WRONLY)) < 0){
			perror("open pipe");
			exit(EXIT_FAILURE);
		}
	} else{
		perror("chiamata: tipologia funzione");
		exit(EXIT_FAILURE);
	}

	// Inizializzazione della stringa di input che rappresenta l'insieme di byte
  // da trasmettere alla central-ECU da parte del processo.
  // La stringa di unsigned char viene tradotta in una stringa di char lunga
	// il doppio più il carattere di terminazione.
  // Ad ogni byte della stringa decodificata corrisponde il carattere ASCII
  // che decodifica la meta` del corrispondente byte della stringa codificata.
  // Quindi per ogni byte della stringa codificata occorreranno 2 byte della
  // stringa decodificata per immagazzinare i caratteri ASCII appropriati.
  // Il +1 nella stringa decodificata  e` necessario per inserirvi il carattere
	// di terminazione riga '\n'.
  unsigned char input_str[INPUT_LEN];
	char input_hex[(INPUT_LEN *2) +1];
	input_hex[INPUT_LEN *2] = '\n';

	// Il ciclo successivo e` il cuore del processo.
	// Una volta al secondo esegue una read sul file descriptor del file di
	// input e, solo se il numero unsigned  char (quindi byte) letti e` uguale
	// al numero specificato in INPUT_LEN, decodifica la stringa letta, la invia
	// sul canale di comunicazione e la scrive nel file di log.
	// Il ciclo e` un ciclo infinito, nel caso caso si utilizzi il processo
	// per ASSIST occorrerà inviare un segnale di interruzione dopo 30
	// secondi per interrompere la lettura e l'invio dei dati.
	while (true) {
		read_conv_broad(input_fd, input_str, input_hex, comm_fd, log_fd);
  }
}

void signal_stp_handler(int sig) {
	fflush(fdopen(comm_fd, "w"));
}
