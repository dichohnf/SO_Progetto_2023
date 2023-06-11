#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

// MACROS
// INPUT_MAX_LEN: lunghezza della stringa di input dalla central-ECU: "FRENO 5\n"
#define INPUT_MAX_LEN 8
// LOG_PHRASE_LEN: lunghezza della stringa di log: "DD/MM/YYYY hh:mm:ss - FRENO 5\n"
#define LOG_PHRASE_LEN 30

// File descriptor del log file
short int log_fd;

void log_func ( );
void emergency_arrest ( );

//La funzione main esegue le operazioni relative al componente brake-by-wire.c
int main ( ) {

	/* Connessione del file descriptor del pipe per la comunicazione tra central ECU e brake-by-wire.
	 * Il protocollo impone che il pipe sia creato durante la fase di inizializzazione del processo central-ECU e che
	 * venga aperto in sola lettura dal processo brake-by-wire il quale vi legga al bisogno. */
	short int pipe_fd =  open ("../tmp/brake.pipe", O_RDONLY);

	/* Connessione del file descriptor del log file. Apertura in sola scrittura.
	 * Qualora il file non esista viene creato. Qualora il file sia presente
	 * si mantengono le precedenti scritture. Dato che viene eseguito l'unlink
	 * da parte della central-ECU allora non vi saranno scritture pendenti da
	 * precedenti esecuzioni. */
	log_fd = open("../log/brake.log", O_WRONLY | O_APPEND | O_CREAT, 0644);

	/* Nel caso sia inviato dalla central ECU un segnale di "PERICOLO",
	 * rappresentato da SIGUSR1, la funzione handler che simula l'arresto
	 * dell'auto e` la procedura emergency_arrest */
	signal (SIGUSR1, &emergency_arrest);

	/* Inizializzazione della stringa di input che rappresenta il comando
	 * impartito dalla central-ECU tramite una stringa di caratteri. */
	char increment[INPUT_MAX_LEN];
	int nread;

  /* Il ciclo infinito successivo rappresenta il cuore del processo.
   * Ad ogni lettura del del pipe potra` accadere:
   * che il pipe in scrittura non sia ancora stato aperto (nread = -1),
   * che il pipe sia aperto ma che non vi sia stato ancora scritto niente (nread = 0),
   * che nel pipe sia stata inserita la stringa "FRENO 5\n" (nread > 0) che rappresenta
   * la richiesta di decremento della velocita' da parte della central-ECU.
   * Quest'ultimo caso da' inizio alla procedura simulativa dell'freno.
   * Il pipe e` bloccante quindi il processo attende un messaggio dalla central-ECU */
	while(1){
		if((nread = read (pipe_fd, &increment, INPUT_MAX_LEN)) > 0){
			log_func( );
		}
	}
}

/* La funzione di logging alloca una stringa come buffer per poter scrivere nel file di log
 * la data attuale, comprensiva dell'orario, e la stringa "FRENO 5\n" */
void log_func ( ){
	// Allocazione del buffer per la formattazione della stringa da inserire nel log file
	char *log_phrase = malloc(LOG_PHRASE_LEN);

  /* Dichiarazione di raw_time, un tempo aritmtico utile per la costruzione di
   * act_time, e inizializzazione di questo al tempo aritmetico relativo al
   * momento attuale di esecuzione time(NULL)*/
	time_t raw_time = time(NULL);

  // Struttura di time.h che mantiene campi distinti per ogni campo della data e dell'orario
	struct tm act_time = *localtime(&raw_time);

  // Riempiemnto del buffer secondo il formato definito
	sprintf(log_phrase, "%02d/%02d/%d %02d:%02d:%02d - FRENO 5\n",
          act_time.tm_mday, act_time.tm_mon, act_time.tm_year + 1900,
          act_time.tm_hour, act_time.tm_min, act_time.tm_sec);

  // Scrittura nel log file
	write(log_fd, log_phrase, LOG_PHRASE_LEN);
	free (log_phrase);
	return;
}

/* La funzione emergency_arrest simula una sequenza di arresto immediato
 * in caso di pericolo ricevuto dalla central-ECU attraverso la scrittura
 * nel log file della stringa "ARRESTO AUTO\n".
 * Questa costituisce il gestore (o handler) del segnale di pericolo
 * da parte della central-ECU. */
void  emergency_arrest ( ){
	write(log_fd, "ARRESTO AUTO\n", 13);
}
