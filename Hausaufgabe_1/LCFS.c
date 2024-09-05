#include "../lib/LCFS.h"


//Oskar Pavliuk, Matrikelnummer 0497851
static queue_object *LCFS_queue;

int stack_push(void *new_object, queue_object *stack);

void *stack_pop(queue_object *stack);

// Hier initialisieren wir unsere Warteschlange
int LCFS_startup() {
    LCFS_queue = new_queue();
    if (NULL == LCFS_queue) {
        return 1;
    }
    return 0;
}

process *LCFS_tick(process *running_process) {
    // Falls running_process gleich NULL ist, oder running_process schon fertig ist, dann von Stack löschen
    if (NULL == running_process || running_process->time_left == 0) {
        running_process = stack_pop(LCFS_queue);
    }
    // Falls running_process nicht NULL ist und noch läuft, dann time_left--
    if (running_process != NULL && running_process->time_left > 0) {
        running_process->time_left--;
    }

    return running_process;
}


process *LCFS_new_arrival(process *arriving_process, process *running_process) {
    // Falls wir einen neuen Process haben, der nicht NULL ist, dann in Stack hinzufügen
    if (arriving_process != NULL) {
        stack_push(arriving_process, LCFS_queue);
    }
    return running_process;
}


void LCFS_finish() {
    // Speicher freigeben
    if (LCFS_queue != NULL) {
        free_queue(LCFS_queue);
    }
}

// Hier implementieren wir Stack, damit wir besser mit diesem Verfahren interagieren können
int stack_push(void *new_object, queue_object *stack) {
    // Falls stack oder new_object gleich NULL ist, dann 1 zurückgeben
    if (NULL == stack || NULL == new_object) {
        return 1;
    }

    // Wir allozieren Speicher für Stack object
    queue_object *new_stack_object = (queue_object *) malloc(sizeof(queue_object));
    if (NULL == new_stack_object) {
        return 1;
    }

    /* Hier fügen wir unseren Prozess hinzu und müssen bestimmte Verbindungen herstellen,
    damit der Prozess weiß, wohin er geht
    */
    new_stack_object->object = new_object;
    new_stack_object->next = stack->next;
    stack->next = new_stack_object;
    return 0;
}

void *stack_pop(queue_object *stack) {
    // Falls stack gleich NULL ist oder wir keine Elemente in Stack haben, dann NULL zurückgeben
    if (NULL == stack || NULL == stack->next) {
        return NULL;
    }

    /*
    Hier hingegen löschen wir den Prozess, aber hier müssen wir die Verbindungen neu anordnen
    */
    queue_object *stack_object = stack->next;
    void *process = stack_object->object;
    stack->next = stack_object->next;
    free(stack_object);
    return process;
}
