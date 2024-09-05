#include "../lib/PRIOP.h"
#include <stdio.h>


//Oskar Pavliuk, Matrikelnummer 0497851
static queue_object *PRIOP_queue;

int queue_add_with_priority(void *new_object, queue_object *queue);

process *PRIOP_tick(process *running_process) {
    /*
    Wenn wir einen Prozess in der Warteschlange haben, der eine höhere Priorität hat als der Prozess,
    der gerade ausgeführt wird, dann tauschen sie die Plätze
    */
    if (running_process != NULL && running_process->time_left > 0) {
        process *next_process = queue_peek(PRIOP_queue);
        if (next_process != NULL && next_process->priority > running_process->priority) {
            queue_add_with_priority(running_process, PRIOP_queue);
            running_process = queue_poll(PRIOP_queue);
        }
    }
    /*
    Wenn der Prozess seine Arbeit beendet hat, beginnen wir mit einem neuen Prozess
    */
    if (NULL == running_process || running_process->time_left == 0) {
        running_process = queue_poll(PRIOP_queue);
    }
    /*
    Wir reduzieren die Zeit für den Prozess
    */
    if (running_process != NULL && running_process->time_left > 0) {
        running_process->time_left--;
    }
    return running_process;
}

// Hier initialisieren wir unsere Warteschlange
int PRIOP_startup() {
    PRIOP_queue = new_queue();
    if (NULL == PRIOP_queue) {
        return 1;
    }
    return 0;
}


process *PRIOP_new_arrival(process *arriving_process, process *running_process) {
    if (NULL == arriving_process) {
        return running_process;
    }

    if (NULL == running_process) {
        return arriving_process;
    }
    /*
    Hat der ankommende Prozess eine höhere Priorität als der ausführende, tauschen sie sozusagen die Plätze
    */
    if (arriving_process->priority > running_process->priority && running_process->time_left > 0) {
        queue_add_with_priority(running_process, PRIOP_queue);
        return arriving_process;
    } else {
        queue_add_with_priority(arriving_process, PRIOP_queue);
        return running_process;
    }
}

void PRIOP_finish() {
    // Speicher freigeben
    if (PRIOP_queue != NULL) {
        free_queue(PRIOP_queue);
    }
}

/*
Dies ist eine Hilfsfunktion, die einen Prozess an der richtigen Stelle in die Warteschlange stellt
(nach Priorität)
*/
int queue_add_with_priority(void *new_object, queue_object *queue) {
    if (NULL == new_object || NULL == queue) {
        return 1;
    }

    queue_object *new_queue_object = (queue_object *) malloc(sizeof(queue_object));
    if (NULL == new_queue_object) {
        return 1;
    }

    new_queue_object->object = new_object;
    new_queue_object->next = NULL;

    queue_object *current = queue;
    process *new_process = (process *) new_object;

    while (current->next != NULL && ((process *) current->next->object)->priority >= new_process->priority) {
        current = current->next;
    }

    new_queue_object->next = current->next;
    current->next = new_queue_object;
    return 0;
}

