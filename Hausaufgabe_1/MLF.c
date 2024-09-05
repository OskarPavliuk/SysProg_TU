#include "../lib/MLF.h"
#include <stdio.h>


//Oskar Pavliuk, Matrikelnummer 0497851
static queue_object **MLF_queues;
static int *quantums;
static int level_i;
static int tick_time;

int power_of_two(int exponent);

int queue_add_with_prioritymlf(void *new_object, queue_object *queue);

process *MLF_tick(process *running_process) {
    /*
    Wir reduzieren die Zeit für den Prozess
    */
    if (running_process != NULL && running_process->time_left > 0) {
        running_process->time_left--;
        tick_time++;
    }
    /*
    Wenn der Prozess bereits beendet ist, dann NULL
    */
    if (running_process != NULL && running_process->time_left == 0) {
        running_process = NULL;
        tick_time = 0;
    }
    /*
    Befindet sich der Prozess auf Stufe 3, also 4 (wenn man von 0 an zählt), dann beendet er sich selbst,
    solange er noch Zeit hat
    */
    if (level_i == 3 && running_process != NULL && running_process->time_left > 0) {
        return running_process;
    }
    /*
    Wenn der Prozess eine bestimmte Anzahl von Malen ausgeführt wurde, gelangt er zur nächsten Ebene
    */
    if (tick_time == quantums[level_i]) {
        if (level_i < 3) {
            queue_add_with_prioritymlf(running_process, MLF_queues[level_i + 1]);
        }
        if (level_i >= 3) {
            queue_add_with_prioritymlf(running_process, MLF_queues[level_i]);
        }
        running_process = NULL;
        tick_time = 0;
    }

    if (NULL == running_process) {
        for (int i = 0; i < 4; i++) {
            if (MLF_queues[i] != NULL && MLF_queues[i]->next != NULL) {
                level_i = i;
                tick_time = 0;
                running_process = queue_poll(MLF_queues[i]);
                break;
            }
        }
    }

    return running_process;
}

/*
Wir initialisieren unsere Warteschlangen sowie das Quantenarray
*/
int MLF_startup() {
    MLF_queues = (queue_object **) malloc(4 * sizeof(queue_object *));
    if (NULL == MLF_queues) {
        return 1;
    }
    quantums = (int *) malloc(4 * sizeof(int));
    if (NULL == quantums) {
        return 1;
    }

    for (int i = 0; i < 4; i++) {
        MLF_queues[i] = new_queue();
        if (NULL == MLF_queues[i]) {
            return 1;
        }
        quantums[i] = power_of_two(i);
    }

    level_i = 0;
    tick_time = 0;
    return 0;
}

/*
Wenn ein neues Objekt erscheint, wechselt es auf Ebene 0
*/
process *MLF_new_arrival(process *arriving_process, process *running_process) {
    if (arriving_process == NULL) {
        return running_process;
    }

    queue_object *first_queue = *MLF_queues;
    queue_add_with_prioritymlf(arriving_process, first_queue);
    return running_process;
}

//Speicher freigeben
void MLF_finish() {
    if (MLF_queues != NULL) {
        for (int i = 0; i < 4; i++) {
            if (MLF_queues[i] != NULL) {
                free_queue(MLF_queues[i]);
            }
        }
        free(MLF_queues);
    }
    if (quantums != NULL) {
        free(quantums);
    }
}


int power_of_two(int exponent) {
    int result = 1;
    for (int i = 0; i < exponent; i++) {
        result *= 2;
    }
    return result;
}

/*
Dies ist eine Hilfsfunktion, die einen Prozess an der richtigen Stelle in die Warteschlange stellt
(nach Priorität)
*/
int queue_add_with_prioritymlf(void *new_object, queue_object *queue) {
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

    while (current->next != NULL && ((process *) current->next->object)->priority >= new_process->priority) {
        current = current->next;
    }
    new_queue_object->next = current->next;
    current->next = new_queue_object;
    return 0;
}
