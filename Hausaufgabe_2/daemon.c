#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "../include/daemon.h"
#include "../include/ringbuf.h"

/* IN THE FOLLOWING IS THE CODE PROVIDED FOR YOU 
 * changing the code will result in points deduction */

/********************************************************************
* NETWORK TRAFFIC SIMULATION: 
* This section simulates incoming messages from various ports using 
* files. Think of these input files as data sent by clients over the
* network to our computer. The data isn't transmitted in a single 
* large file but arrives in multiple small packets. This concept
* is discussed in more detail in the advanced module: 
* Rechnernetze und Verteilte Systeme
*
* To simulate this parallel packet-based data transmission, we use multiple 
* threads. Each thread reads small segments of the files and writes these 
* smaller packets into the ring buffer. Between each packet, the
* thread sleeps for a random time between 1 and 100 us. This sleep
* simulates that data packets take varying amounts of time to arrive.
*********************************************************************/
typedef struct {
    rbctx_t *ctx;
    connection_t *connection;
} w_thread_args_t;

void *write_packets(void *arg) {
    /* extract arguments */
    rbctx_t *ctx = ((w_thread_args_t *) arg)->ctx;
    size_t from = (size_t) ((w_thread_args_t *) arg)->connection->from;
    size_t to = (size_t) ((w_thread_args_t *) arg)->connection->to;
    char *filename = ((w_thread_args_t *) arg)->connection->filename;

    /* open file */
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open file with name %s\n", filename);
        exit(1);
    }

    /* read file in chunks and write to ringbuffer with random delay */
    unsigned char buf[MESSAGE_SIZE];
    size_t packet_id = 0;
    size_t read = 1;
    while (read > 0) {
        size_t msg_size = MESSAGE_SIZE - 3 * sizeof(size_t);
        read = fread(buf + 3 * sizeof(size_t), 1, msg_size, fp);
        if (read > 0) {
            memcpy(buf, &from, sizeof(size_t));
            memcpy(buf + sizeof(size_t), &to, sizeof(size_t));
            memcpy(buf + 2 * sizeof(size_t), &packet_id, sizeof(size_t));
            while (ringbuffer_write(ctx, buf, read + 3 * sizeof(size_t)) != SUCCESS) {
                usleep(((rand() % 50) + 25)); // sleep for a random time between 25 and 75 us
            }
        }
        packet_id++;
        usleep(((rand() % (100 - 1)) + 1)); // sleep for a random time between 1 and 100 us
    }
    fclose(fp);
    return NULL;
}

/* END OF PROVIDED CODE */


/********************************************************************/

/* YOUR CODE STARTS HERE */
// Oskar Pavliuk Matrikelnummer 0497851
typedef struct {
    rbctx_t *ctx; // Kontext des Ringpuffers
    pthread_cond_t *file_wait_conditions[MAXIMUM_PORT + 1]; // Bedingungsvariablen für Dateien
    pthread_mutex_t *packet_mutex[MAXIMUM_PORT + 1]; // Mutex für Paket
    size_t *id_von_packet; // Paket-IDs
} r_thread_args_t;

// Funktion zum Kopieren von Daten
void copy_data_for_daemon(void *dst, void *src, size_t size) {
    unsigned char *destination = dst;
    unsigned char *source = src;
    for (size_t i = 0; i < size; i++) {
        destination[i] = source[i]; // Byteweise Kopie
    }
}

typedef enum {
    NOT_FOUND,
    FOUND
} CheckResultForFunction;

// Funktion zur Überprüfung auf das Wort "malicious" in der Nachricht
CheckResultForFunction malicious_check(const char *message, size_t message_len) {
    char *malicious = "malicious";
    size_t j = 0;
    for (size_t i = 0; i < message_len; i++) {
        if (message[i] == malicious[j]) {
            j++;
            if (j == strlen(malicious)) {
                return FOUND; // Gibt FOUND zurück, wenn das Wort gefunden wurde
            }
        }
    }
    return NOT_FOUND; // Gibt NOT_FOUND zurück, wenn das Wort nicht gefunden wurde
}

// Funktion zum Filtern von Nachrichten
int filter_message(size_t from, size_t to, char *message, size_t message_len) {
    return (malicious_check(message, message_len) || from == to || from == 42 || to == 42 || from + to == 42);
    // Gibt 1 zurück, wenn die Nachricht gefiltert werden soll
}

// Funktion zum Schreiben der Nachricht in eine Datei
void write_message_to_file(size_t to, char *message, size_t message_len) {
    char filename[50];
    snprintf(filename, sizeof(filename), "%zu.txt", to); // Erstellen des Dateinamens
    FILE *fp = fopen(filename, "a+"); // Öffnen der Datei
    if (fp) {
        fwrite(message, 1, message_len, fp); // Schreiben der Nachricht in die Datei
        fclose(fp); // Schließen der Datei
    }
    if (fp == NULL) {
        fprintf(stderr, "Cannot open file with name %s\n", filename);
        exit(1); // Beenden des Programms, wenn die Datei nicht geöffnet werden konnte
    }
}

void *read_packets(void *arg) {
    r_thread_args_t *r_args = (r_thread_args_t *) arg;
    rbctx_t *ctx = r_args->ctx;
    pthread_cond_t *file_wait_conditions = (pthread_cond_t *) r_args->file_wait_conditions;
    pthread_mutex_t *packet_mutex = (pthread_mutex_t *) r_args->packet_mutex;
    size_t *id_von_packet = r_args->id_von_packet;

    unsigned char buffer[MESSAGE_SIZE]; // Puffer für gelesene Daten
    size_t buf_length = MESSAGE_SIZE; // Länge des Puffers
    int variable_for_read_bedingung = 1; // Variable für die Schleifenbedingung

    do {
        int read_status = ringbuffer_read(ctx, buffer, &buf_length); // Lesen aus dem Ringpuffer
        if (read_status == SUCCESS) {
            size_t from = 0, to = 0, id_packet = 0;
            // Daten kopieren
            copy_data_for_daemon(&from, buffer, sizeof(size_t));
            copy_data_for_daemon(&to, buffer + sizeof(size_t), sizeof(size_t));
            copy_data_for_daemon(&id_packet, buffer + 2 * sizeof(size_t), sizeof(size_t));
            char *message = (char *) (buffer + 3 * sizeof(size_t));

            pthread_mutex_lock((pthread_mutex_t *) &packet_mutex[from - 1]); // Mutex für das aktuelle Paket sperren
            if (filter_message(from, to, message, buf_length - 3 * sizeof(size_t))) {
                id_von_packet[from - 1] = id_von_packet[from - 1] + 1;
                pthread_mutex_unlock((pthread_mutex_t *) &packet_mutex[from - 1]); //unlock Mutex
                pthread_cond_broadcast(
                        (pthread_cond_t *) &file_wait_conditions[from - 1]); // Bedingungsvariable signalisieren
                continue;
            }

            while (id_packet != id_von_packet[from - 1]) {
                pthread_cond_wait((pthread_cond_t *) &file_wait_conditions[from - 1],
                                  (pthread_mutex_t *) &packet_mutex[from - 1]); // Auf das richtige Paket warten
            }

            write_message_to_file(to, message, buf_length - 3 * sizeof(size_t)); // Nachricht in die Datei schreiben
            id_von_packet[from - 1] = id_von_packet[from - 1] + 1;
            pthread_mutex_unlock((pthread_mutex_t *) &packet_mutex[from - 1]); // unlock Mutex
            buf_length = MESSAGE_SIZE;
            pthread_cond_broadcast(
                    (pthread_cond_t *) &file_wait_conditions[from - 1]); // Bedingungsvariable signalisieren
        }
        if (read_status != SUCCESS) {
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
            if (buf_length != MESSAGE_SIZE) {
                buf_length = MESSAGE_SIZE;
            }
            usleep(((rand() % 25) + 25)); // Warten
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
            continue;
        }
    } while (variable_for_read_bedingung > 0);

    return NULL;
}



// 1. read functionality
// 2. filtering functionality
// 3. (thread-safe) write to file functionality

/* YOUR CODE ENDS HERE */

/********************************************************************/

int simpledaemon(connection_t *connections, int nr_of_connections) {
    /* initialize ringbuffer */
    rbctx_t rb_ctx;
    size_t rbuf_size = 1024;
    void *rbuf = malloc(rbuf_size);
    if (rbuf == NULL) {
        fprintf(stderr, "Error allocation ringbuffer\n");
    }

    ringbuffer_init(&rb_ctx, rbuf, rbuf_size);

    /****************************************************************
    * WRITER THREADS 
    * ***************************************************************/

    /* prepare writer thread arguments */
    w_thread_args_t w_thread_args[nr_of_connections];
    for (int i = 0; i < nr_of_connections; i++) {
        w_thread_args[i].ctx = &rb_ctx;
        w_thread_args[i].connection = &connections[i];
        /* guarantee that port numbers range from MINIMUM_PORT (0) - MAXIMUMPORT */
        if (connections[i].from > MAXIMUM_PORT || connections[i].to > MAXIMUM_PORT ||
            connections[i].from < MINIMUM_PORT || connections[i].to < MINIMUM_PORT) {
            fprintf(stderr, "Port numbers %d and/or %d are too large\n", connections[i].from, connections[i].to);
            exit(1);
        }
    }

    /* start writer threads */
    pthread_t w_threads[nr_of_connections];
    for (int i = 0; i < nr_of_connections; i++) {
        pthread_create(&w_threads[i], NULL, write_packets, &w_thread_args[i]);
    }

    /****************************************************************
    * READER THREADS
    * ***************************************************************/

    pthread_t r_threads[NUMBER_OF_PROCESSING_THREADS];

    /* END OF PROVIDED CODE */

    /********************************************************************/

    /* YOUR CODE STARTS HERE */
    // Initialisierung der Reader-Thread-Argumente
    r_thread_args_t r_thread_args[NUMBER_OF_PROCESSING_THREADS];
    pthread_cond_t *file_wait_conditions[MAXIMUM_PORT + 1];
    pthread_mutex_t *packet_mutex[MAXIMUM_PORT + 1];
    size_t id_von_packet[MAXIMUM_PORT + 1];

    // Initialisierung der Bedingungsvariablen, Mutexes und Paket-IDs
    for (int i = 0; i < MAXIMUM_PORT + 1; i++) {
        file_wait_conditions[i] = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
        pthread_cond_init(file_wait_conditions[i], NULL);
        packet_mutex[i] = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
        pthread_mutex_init(packet_mutex[i], NULL);
        id_von_packet[i] = 0;
    }

    // Konfiguration der Argumente für die Reader-Threads
    for (int i = 0; i < NUMBER_OF_PROCESSING_THREADS; i++) {
        r_thread_args[i].ctx = &rb_ctx; // Zuweisung des Ringpuffer-Kontexts
        r_thread_args[i].id_von_packet = id_von_packet; // Zuweisung der Paket-IDs
        // Zuweisung der Bedingungsvariablen und Mutexes für jeden Port
        for (size_t j = 0; j < MAXIMUM_PORT + 1; j++) {
            r_thread_args[i].file_wait_conditions[j] = file_wait_conditions[j];
            r_thread_args[i].packet_mutex[j] = packet_mutex[j];
        }
    }

    // Start der Reader-Threads
    for (int i = 0; i < NUMBER_OF_PROCESSING_THREADS; i++) {
        pthread_create(&r_threads[i], NULL, read_packets, &r_thread_args[i]); // Erstellung der Reader-Threads
    }

    // 1. think about what arguments you need to pass to the processing threads
    // 2. start the processing threads

    /* YOUR CODE ENDS HERE */

    /********************************************************************/



    /* IN THE FOLLOWING IS THE CODE PROVIDED FOR YOU 
     * changing the code will result in points deduction */

    /****************************************************************
     * CLEANUP
     * ***************************************************************/

    /* after 5 seconds JOIN all threads (we should definitely have received all messages by then) */
    printf("daemon: waiting for 5 seconds before canceling reading threads\n");
    sleep(5);
    for (int i = 0; i < NUMBER_OF_PROCESSING_THREADS; i++) {
        pthread_cancel(r_threads[i]);
    }

    /* wait for all threads to finish */
    for (int i = 0; i < nr_of_connections; i++) {
        pthread_join(w_threads[i], NULL);
    }

    /* join all threads */
    for (int i = 0; i < NUMBER_OF_PROCESSING_THREADS; i++) {
        pthread_join(r_threads[i], NULL);
    }

    /* END OF PROVIDED CODE */



    /********************************************************************/

    /* YOUR CODE STARTS HERE */

    // use this section to free any memory, destory mutexe etc.
    // Freigabe der Bedingungsvariablen und Mutexes
    for (int i = 0; i < MAXIMUM_PORT + 1; i++) {
        if (file_wait_conditions[i] != NULL) {
            pthread_cond_destroy(file_wait_conditions[i]); // Zerstörung der Bedingungsvariablen
            free(file_wait_conditions[i]); // Freigabe des Speichers der Bedingungsvariablen
        }
        if (packet_mutex[i] != NULL) {
            pthread_mutex_destroy(packet_mutex[i]); // Zerstörung des Mutexes
            free(packet_mutex[i]); // Zerstörung des Mutexes
        }
    }
    /* YOUR CODE ENDS HERE */

    /********************************************************************/



    /* IN THE FOLLOWING IS THE CODE PROVIDED FOR YOU 
    * changing the code will result in points deduction */

    free(rbuf);
    ringbuffer_destroy(&rb_ctx);

    return 0;

    /* END OF PROVIDED CODE */
}