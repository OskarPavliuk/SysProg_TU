#include "../lib/HRRN.h"


//Oskar Pavliuk, Matrikelnummer 0497851
static queue_object *HRRN_queue;
static int current_time = 0;

float calculate_rr(HRRN_process *rr_process);

HRRN_process *find_highest_rr_process(queue_object **highest_rr_previous);


process *HRRN_tick(process *running_process) {
    if (running_process == NULL || running_process->time_left == 0) {
        if (running_process != NULL && running_process->time_left == 0) {
            running_process = NULL;
        }

        queue_object *highest_rr_previous = HRRN_queue;
        HRRN_process *highest_rr_process = find_highest_rr_process(&highest_rr_previous);

        if (highest_rr_process != NULL) {
            highest_rr_previous->next = highest_rr_previous->next->next;
            running_process = highest_rr_process->this_process;
        }
    }

    if (running_process != NULL) {
        running_process->time_left--;
    }

    current_time++;
    return running_process;
}

// Hier initialisieren wir unsere Warteschlange
int HRRN_startup() {
    HRRN_queue = new_queue();
    if (HRRN_queue == NULL) {
        return 1;
    }
    current_time = 0;
    return 0;
}

/*
Wenn ein Prozess eintrifft, initialisieren wir einige Werte dafür und fügen ihn der Warteschlange hinzu
*/
process *HRRN_new_arrival(process *arriving_process, process *running_process) {
    if (arriving_process == NULL) {
        return running_process;
    }

    HRRN_process *rr_process = (HRRN_process *) malloc(sizeof(HRRN_process));
    if (rr_process == NULL) {
        return running_process;
    }

    rr_process->this_process = arriving_process;
    rr_process->waiting_time = 0;
    rr_process->rr = 0.0;

    arriving_process->start_time = current_time;
    queue_add(rr_process, HRRN_queue);
    return running_process;
}

// Speicher freigeben
void HRRN_finish() {
    if (HRRN_queue != NULL) {
        free_queue(HRRN_queue);
    }
}

/*
Hier berechnen wir den RR für den Prozess anhand der Formel. Ich habe die Formel aus Vorlesung 3 übernommen
*/
float calculate_rr(HRRN_process *rr_process) {
    if (rr_process == NULL || rr_process->this_process == NULL) {
        return -1.0;
    }

    rr_process->waiting_time = current_time - rr_process->this_process->start_time;
    return (float) (rr_process->waiting_time + rr_process->this_process->time_left) /
           rr_process->this_process->time_left;
}


/*
Dies ist eine Hilfsfunktion, die uns den Prozess mit der höchsten RR findet und ihn zurückgibt
*/
HRRN_process *find_highest_rr_process(queue_object **highest_rr_previous) {
    HRRN_process *highest_rr_process = NULL;
    queue_object *current = HRRN_queue->next;
    queue_object *previous = HRRN_queue;

    while (current != NULL) {
        HRRN_process *rr_process = (HRRN_process *) current->object;
        rr_process->rr = calculate_rr(rr_process);

        if (highest_rr_process == NULL || rr_process->rr > highest_rr_process->rr ||
            (rr_process->rr == highest_rr_process->rr &&
             rr_process->this_process->start_time < highest_rr_process->this_process->start_time)) {
            highest_rr_process = rr_process;
            *highest_rr_previous = previous;
        }
        previous = current;
        current = current->next;
    }

    return highest_rr_process;
}