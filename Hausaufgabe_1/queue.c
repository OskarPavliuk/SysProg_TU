#include "../lib/queue.h"
#include <stdlib.h>
#include <stdio.h>


//Oskar Pavliuk, Matrikelnummer 0497851
int queue_add(void *new_object, queue_object *queue) {
    // Falls queue oder unser object gleich NULL ist, dann 1 zurückgeben
    if (NULL == queue || NULL == new_object) {
        return 1;
    }

    // Hier erstellen wir ein Objekt
    queue_object *new_queue_object = (queue_object *) malloc(sizeof(queue_object));

    // Falls unser Objekt nicht erstellt wurde, dann 1 zurückgeben
    if (NULL == new_queue_object) {
        return 1;
    }

    new_queue_object->object = new_object;
    new_queue_object->next = NULL;

    queue_object *current = queue;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = new_queue_object;

    return 0;
}


void *queue_poll(queue_object *queue) {
    // Falls queue oder wir keine Elemente in queue haben, dann NULL zurückgeben
    if (NULL == queue || NULL == queue->next) {
        return NULL;
    }

    /*
    Wir löschen den Prozess und ändern die Verbindungen zwischen den Prozessen
    */
    queue_object *poll_element = queue->next;
    queue->next = poll_element->next;
    void *object = poll_element->object;
    free(poll_element);
    return object;
}

queue_object *new_queue() {
    // Wir allozieren den Speicher für queue
    queue_object *queue = (queue_object *) malloc(sizeof(queue_object));

    // Falls queue gleich NULL ist, dann NULL zurückgeben
    if (NULL == queue) {
        return NULL;
    }

    // erstes Element ist gleich NULL, queue ist leer
    queue->next = NULL;
    queue->object = NULL;

    return queue;
}


void free_queue(queue_object *queue) {
    // Falls queue gleich NULL ist, oder wir keine Elemente in queue haben, dann return;
    if (NULL == queue || NULL == queue->next) {
        return;
    }

    // Ansonsten müssen wir jedes Element löschen und Speicher dementsprechend freigeben
    queue_object *current = queue;
    queue_object *next;

    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
}


void *queue_peek(queue_object *queue) {
    // Falls queue gleich NULL ist, oder wir keine Elemente in queue haben, dann return NULL
    if (NULL == queue || NULL == queue->next) {
        return NULL;
    }
    // Wir müssen das erste Element von queue zurückgeben (queue(header)->next zeigt auf erstes_element)
    queue_object *peek_element = queue->next->object;

    return peek_element;
}

