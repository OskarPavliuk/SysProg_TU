#include "../lib/RR.h"


//Oskar Pavliuk, Matrikelnummer 0497851
static queue_object *RR_queue;
static int RR_quantum;
static int current_quantum;


process *RR_tick(process *running_process) {
    // Falls queue gleich NULL ist, dann NULL zurückgeben
    if (NULL == RR_queue) {
        return NULL;
    }

    /*
    Hier verkürzen wir die Ausführungszeit des Prozesses und erhöhen das aktuelle Quantum
    */
    if (running_process != NULL && running_process->time_left > 0) {
        running_process->time_left--;
        current_quantum++;
    }
    /*
    Hier prüfen wir, ob der Prozess die erforderliche Distanz bereits zurückgelegt hat und
    ob er später ausgeführt werden soll. Wenn ja, fügen wir ihn der Warteschlange hinzu und
    nehmen das nächste Element aus der Warteschlange
    */
    if (current_quantum == RR_quantum && running_process->time_left > 0) {
        queue_add(running_process, RR_queue);
        running_process = queue_poll(RR_queue);
        current_quantum = 0;
    }
    /*
    Hier prüfen wir, ob die Ausführung des Prozesses bereits gestoppt wurde und wenn ja,
    gehen wir zum nächsten Prozess über
    */
    if (NULL == running_process || running_process->time_left == 0) {
        running_process = queue_poll(RR_queue);
        current_quantum = 0;
    }
    return running_process;
}


/*
Hier initialisieren wir unsere Warteschlange sowie das Gesamtquantum, das uns sagt,
wie oft jeder Prozess insgesamt ausgeführt werden soll, und auch das Quantum, das uns sagt,
wie oft der Prozess bereits ausgeführt wurde
*/
int RR_startup(int quantum) {
    RR_queue = new_queue();
    if (NULL == RR_queue) {
        return 1;
    }
    if (quantum <= 0) {
        return 1;
    }
    RR_quantum = quantum;
    current_quantum = 0;
    return 0;
}

/*
Hier prüfen wir einfach, ob ein Prozess ankommt, dann geht er sofort in die Warteschlange,
da der Algorithmus selbst nicht davon ausgeht, dass Prozesse sich gegenseitig unterbrechen können
*/
process *RR_new_arrival(process *arriving_process, process *running_process) {
    if (arriving_process != NULL) {
        queue_add(arriving_process, RR_queue);
    }

    return running_process;
}

/*
Hier geben wir Ressourcen für unsere Warteschlange frei
*/
void RR_finish() {
    if (RR_queue != NULL) {
        free_queue(RR_queue);
    }
}
