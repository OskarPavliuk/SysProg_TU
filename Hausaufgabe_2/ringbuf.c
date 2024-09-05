#include "../include/ringbuf.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

// Oskar Pavliuk Matrikelnummer 0497851
void ringbuffer_init(rbctx_t *context, void *buffer_location, size_t buffer_size) {
    context->begin = buffer_location; // Anfang des Puffers festlegen
    context->end = buffer_location + buffer_size; // Ende des Puffers festlegen
    context->read = context->begin; // Lesezeiger auf den Anfang setzen
    context->write = context->begin; // Schreibzeiger auf den Anfang setzen

    pthread_mutex_init(&context->mtx, NULL); // Mutex initialisieren
    pthread_cond_init(&context->sig, NULL); // Bedingungsvariable initialisieren
}

// -----------------------------------------------------------------------------
void copy_data(uint8_t *dest, const uint8_t *src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        dest[i] = src[i]; // Daten byteweise kopieren
    }
}

void wrap_around_check(rbctx_t *context) {
    // Wenn das Ende des Puffers erreicht ist, zum Anfang zurückkehren
    if (context->write == context->end) {
        context->write = context->begin;
    }
}

void wrap_around_read_check(rbctx_t *context) {
    // Wenn das Ende des Puffers erreicht ist, zum Anfang zurückkehren
    if (context->read == context->end) {
        context->read = context->begin;
    }
}

size_t calculate_free_space(rbctx_t *context) {
    size_t whole_space_to_go = 0;

    // Überprüfen, ob der Schreibzeiger hinter dem Lesezeiger liegt
    if (context->write > context->read) {
        whole_space_to_go = (context->end - context->write) + (context->read - context->begin) - 1;
    }

    // Überprüfen, ob der Schreibzeiger vor dem Lesezeiger liegt
    if (context->write < context->read) {
        whole_space_to_go = (context->read - context->write) - 1;
    }

    // Überprüfen, ob der Schreibzeiger gleich dem Lesezeiger ist
    if (context->write == context->read) {
        whole_space_to_go = (context->end - context->write) + (context->read - context->begin) - 1;
    }

    return whole_space_to_go;
}


void write(rbctx_t *context, const uint8_t *data, size_t data_len) {
    size_t space_to_the_end_left = context->end - context->write;

    // Überprüfen, ob genügend Platz bis zum Ende des Puffers vorhanden ist
    if (data_len <= space_to_the_end_left) {
        copy_data(context->write, data, data_len);
        context->write = context->write + data_len;
        wrap_around_check(context);
    }
    if (data_len > space_to_the_end_left) {
        // Wenn nicht genügend Platz vorhanden ist, führen wir einen Wrap-around durch
        copy_data(context->write, data, space_to_the_end_left);
        copy_data(context->begin, data + space_to_the_end_left, data_len - space_to_the_end_left);
        context->write = context->begin + (data_len - space_to_the_end_left);
        wrap_around_check(context);
    }
}

int ringbuffer_write(rbctx_t *context, void *message, size_t message_len) {
    size_t total_size_of_message = sizeof(size_t) + message_len; // Gesamte Nachrichtengröße berechnen
    size_t whole_space_to_go;
    struct timespec timeout;
    int cond_wait_status;

    pthread_mutex_lock(&context->mtx); // Mutex sperren, um exklusiven Zugriff zu gewährleisten

    if (total_size_of_message > (size_t) (context->end - context->begin)) {
        pthread_mutex_unlock(&context->mtx); // Mutex entsperren
        return RINGBUFFER_FULL;
    }

    do {
        whole_space_to_go = calculate_free_space(context); // Freien Platz im Puffer berechnen

        if (total_size_of_message <= whole_space_to_go) {
            break; // Aus der Schleife aussteigen, wenn genug Platz vorhanden ist
        }

        clock_gettime(CLOCK_REALTIME, &timeout); // Aktuelle Zeit ermitteln
        timeout.tv_sec = timeout.tv_sec + RBUF_TIMEOUT; // Timeout-Zeit hinzufügen

        cond_wait_status = pthread_cond_timedwait(&context->sig, &context->mtx,
                                                  &timeout); // Bedingungsvariable mit Timeout warten
        if (cond_wait_status == ETIMEDOUT) {
            pthread_mutex_unlock(&context->mtx); // Mutex entsperren
            return RINGBUFFER_FULL; // Fehler zurückgeben, wenn Timeout überschritten ist
        }
    } while (total_size_of_message > whole_space_to_go);

    write(context, (uint8_t *) &message_len, sizeof(size_t)); // Nachrichtenlänge in den Puffer schreiben
    write(context, (uint8_t *) message, message_len); // Nachricht in den Puffer schreiben

    pthread_cond_signal(&context->sig); // Signal an wartende Threads senden
    pthread_mutex_unlock(&context->mtx); // Mutex entsperren

    return SUCCESS;
}

// -----------------------------------------------------------------------------

void read_message_length(rbctx_t *context, size_t *message_len) {
    uint8_t *message_len_ptr = (uint8_t *) message_len;
    for (size_t i = 0; i < sizeof(size_t); i++) {
        wrap_around_read_check(context); // Überprüfen, ob der Lesezeiger am Ende ist und zum Anfang zurückkehren
        message_len_ptr[i] = *context->read++; // Byteweise Länge der Nachricht lesen
    }
}

void read_message_data(rbctx_t *context, void *buffer, size_t message_len) {
    for (size_t i = 0; i < message_len; i++) {
        wrap_around_read_check(context); // Überprüfen, ob der Lesezeiger am Ende ist und zum Anfang zurückkehren
        ((uint8_t *) buffer)[i] = *context->read++; // Byteweise Nachrichtendaten lesen
    }
}

int ringbuffer_read(rbctx_t *context, void *buffer, size_t *buffer_len) {
    struct timespec timeout;
    int cond_wait_status;
    size_t message_len = 0;
    pthread_mutex_lock(&context->mtx); // Mutex sperren, um exklusiven Zugriff zu gewährleisten

    do {
        if (context->read != context->write) {
            break; // Aus der Schleife aussteigen, wenn Daten zum Lesen vorhanden sind
        }

        clock_gettime(CLOCK_REALTIME, &timeout); // Aktuelle Zeit ermitteln
        timeout.tv_sec = timeout.tv_sec + RBUF_TIMEOUT; // Timeout-Zeit hinzufügen

        cond_wait_status = pthread_cond_timedwait(&context->sig, &context->mtx,
                                                  &timeout); // Bedingungsvariable mit Timeout warten
        if (cond_wait_status == ETIMEDOUT) {
            pthread_mutex_unlock(&context->mtx); // Mutex entsperren
            return RINGBUFFER_EMPTY; // Fehler zurückgeben, wenn Timeout überschritten ist
        }
    } while (context->read == context->write);

    read_message_length(context, &message_len); // Nachrichtenlänge aus dem Puffer lesen

    if (*buffer_len < message_len) {
        pthread_mutex_unlock(&context->mtx); // Mutex entsperren
        return OUTPUT_BUFFER_TOO_SMALL; // Fehler zurückgeben, wenn der Puffer zu klein ist
    }

    *buffer_len = message_len; // Tatsächliche Nachrichtenlänge im Puffer speichern

    read_message_data(context, buffer, message_len); // Nachrichtendaten aus dem Puffer lesen
    wrap_around_read_check(context); // Überprüfen, ob der Lesezeiger am Ende ist und zum Anfang zurückkehren

    pthread_cond_signal(&context->sig); // Signal an wartende Threads senden
    pthread_mutex_unlock(&context->mtx); // Mutex entsperren

    return SUCCESS;
}

void ringbuffer_destroy(rbctx_t *context) {
    context->begin = NULL; // Anfang des Puffers auf NULL setzen
    context->end = NULL; // Ende des Puffers auf NULL setzen
    context->read = NULL; // Lesezeiger auf NULL setzen
    context->write = NULL; // Schreibzeiger auf NULL setzen

    pthread_mutex_destroy(&context->mtx); // Mutex zerstören
    pthread_cond_destroy(&context->sig); // Bedingungsvariable zerstören
}
