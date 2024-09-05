#include "../lib/SRTN.h"


//Oskar Pavliuk, Matrikelnummer 0497851
static queue_object *SRTN_queue;


int queue_add_with_time_left(void *new_object, queue_object *queue);

/*
Hier ist die Idee im Grunde die gleiche wie bei (PRIOP),
nur nehmen wir statt der Priorität die verbleibende Zeit
*/
process *SRTN_tick(process *running_process) {
    if (running_process != NULL && running_process->time_left > 0) {
        process *next_process = (process *) queue_peek(SRTN_queue);
        if (next_process != NULL && next_process->time_left < running_process->time_left) {
            queue_add_with_time_left(running_process, SRTN_queue);
            running_process = queue_poll(SRTN_queue);
        }
    }

    if (NULL == running_process || running_process->time_left == 0) {
        running_process = queue_poll(SRTN_queue);
    }

    if (running_process != NULL && running_process->time_left > 0) {
        running_process->time_left--;
    }

    return running_process;
}

int SRTN_startup() {
    SRTN_queue = new_queue();
    if (SRTN_queue == NULL) {
        return 1;
    }
    return 0;
}

process *SRTN_new_arrival(process *arriving_process, process *running_process) {
    if (arriving_process == NULL) {
        return running_process;
    }

    if (running_process == NULL) {
        return arriving_process;
    }

    if (arriving_process->time_left < running_process->time_left && running_process->time_left > 0) {
        queue_add_with_time_left(running_process, SRTN_queue);
        return arriving_process;
    } else {
        queue_add_with_time_left(arriving_process, SRTN_queue);
        return running_process;
    }
}

void SRTN_finish() {
    if (SRTN_queue != NULL) {
        free_queue(SRTN_queue);
    }
}


int queue_add_with_time_left(void *new_object, queue_object *queue) {
    if (new_object == NULL || queue == NULL) {
        return 1;
    }

    queue_object *new_queue_object = (queue_object *) malloc(sizeof(queue_object));
    if (new_queue_object == NULL) {
        return 1;
    }
    new_queue_object->object = new_object;
    new_queue_object->next = NULL;

    queue_object *current = queue;
    process *new_process = (process *) new_object;

    while (current->next != NULL && ((process *) current->next->object)->time_left <= new_process->time_left) {
        current = current->next;
    }

    new_queue_object->next = current->next;
    current->next = new_queue_object;
    return 0;
}

