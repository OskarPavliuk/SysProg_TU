#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<limits.h>
#include "../lib/queue.h"
#include "../lib/process.h"
#include "../lib/FCFS.h"
#include "../lib/LCFS.h"
#include "../lib/PRIOP.h"
#include "../lib/RR.h"
#include "../lib/SRTN.h"
#include "../lib/HRRN.h"
#include "../lib/MLF.h"
#include "../lib/colors.h"
#include "../lib/scheduler.h"

static int toggle_color = 1;

static process **arriving_processes;
static unsigned int idle_count;
static char *resulting_schedule;

static void sort_processes_by_start_time(unsigned int process_count)
{
	process **temp = calloc(process_count, sizeof(process *));
	for (unsigned int i = 0; i < process_count; i++) {
		unsigned int lowest_time = UINT_MAX;
		unsigned int lowest_process_ind = 0;
		for (unsigned int j = 0; j < process_count; j++) {
			if (arriving_processes[j] != NULL) {
				if (arriving_processes[j]->start_time <
				    lowest_time) {
					lowest_time =
					    arriving_processes[j]->start_time;
					lowest_process_ind = j;
				}
			}
		}
		temp[i] = arriving_processes[lowest_process_ind];
		arriving_processes[lowest_process_ind] = NULL;
	}
	free(arriving_processes);
	arriving_processes = temp;
}

static void initialize_processes(unsigned int process_count,
				 process processes[])
{
	arriving_processes = calloc(process_count, sizeof(process *));
	for (unsigned int i = 0; i < process_count; i++) {
		arriving_processes[i] = calloc(1, sizeof(process));
		arriving_processes[i]->start_time = processes[i].start_time;
		arriving_processes[i]->time_left = processes[i].time_left;
		arriving_processes[i]->priority = processes[i].priority;
		arriving_processes[i]->id = processes[i].id;
	}
	sort_processes_by_start_time(process_count);
}

static void print_and_save_process(process *running_process, unsigned int time)
{
	if (running_process != NULL) {
		for (unsigned int i = 0; i < idle_count; i++) {
			printf("  |");
		}
		idle_count = 0;
		if (toggle_color)
			set_color(running_process->id);
		printf(" %c ", running_process->id);
		resulting_schedule[time] = running_process->id;
		if (toggle_color)
			reset_color();
		printf("|");
	} else {
		idle_count++;
		resulting_schedule[time] = ' ';
	}
}

unsigned int overlapping_time(unsigned int p_id)
{
	int overlap =
	    (int)end_time(p_id - 1) - (int)arriving_processes[p_id]->start_time;
	if (overlap < 0) {
		return 0;
	}
	return (unsigned int)overlap;
}

unsigned int end_time(unsigned int p_id)
{
	unsigned int local_end =
	    arriving_processes[p_id]->start_time +
	    arriving_processes[p_id]->time_left;
	if (p_id == 0) {
		return local_end;
	} else {
		return local_end + overlapping_time(p_id);
	}
}

static unsigned int calc_max_time(unsigned int process_count)
{
	unsigned int max_time = end_time(process_count - 1);
	resulting_schedule = calloc(max_time + 1, sizeof(char));
	return max_time;
}

//checks for newly arriving processes at current _time_. Return NULL if there is none
static process *check_new_arrival(unsigned int process_count, unsigned int time)
{
	process *new_arrival = NULL;
	for (unsigned int i = 0; i < process_count; i++) {
		if (arriving_processes[i]->start_time == time) {
			new_arrival = arriving_processes[i];
			break;
		}
	}
	return new_arrival;
}

static void free_processes(unsigned int process_count)
{
	for (unsigned int i = 0; i < process_count; i++) {
		free(arriving_processes[i]);
	}
	free(arriving_processes);
}

char *scheduler(process processes[], unsigned int process_count,
		scheduling_strategy strategy, int RR_quantum)
{

	//Read processes into array "process** arriving_processes"
	initialize_processes(process_count, processes);
	process *running_process = NULL;
	unsigned int max_time = calc_max_time(process_count);

	idle_count = 0;

	//Start FCFS Scheduler
	switch (strategy) {
	case FCFS:
		/* code */
		printf("Starting FCFS scheduler\n");
		if (FCFS_startup() != 0) {
			fprintf(stderr,
				"Problem starting the FCFS scheduler\n");
			exit(1);
		}
		for (unsigned int time = 0; time < max_time; time++) {
			//check for new arrival at given point of time
			process *arriving_process =
			    check_new_arrival(process_count, time);

			//Let the FCFS-scheduler handle the processes
			running_process =
			    FCFS_new_arrival(arriving_process, running_process);
			running_process = FCFS_tick(running_process);
			print_and_save_process(running_process, time);
		}
		FCFS_finish();
		break;

	case LCFS:
		printf("Starting LCFS scheduler\n");
		if (LCFS_startup() != 0) {
			fprintf(stderr,
				"Problem starting the LCFS scheduler\n");
			exit(1);
		}
		for (unsigned int time = 0; time < max_time; time++) {
			//check for new arrival at given point of time
			process *arriving_process =
			    check_new_arrival(process_count, time);

			//Let the LCFS-scheduler handle the processes
			running_process =
			    LCFS_new_arrival(arriving_process, running_process);
			running_process = LCFS_tick(running_process);
			print_and_save_process(running_process, time);
		}
		LCFS_finish();
		break;

	case PRIOP:
		printf("Starting PRIOP scheduler\n");
		if (PRIOP_startup() != 0) {
			fprintf(stderr,
				"Problem starting the PRIOP scheduler\n");
			exit(1);
		}
		for (unsigned int time = 0; time < max_time; time++) {
			//check for new arrival at given point of time
			process *arriving_process =
			    check_new_arrival(process_count, time);

			//Let the PRIOP-scheduler handle the processes
			running_process =
			    PRIOP_new_arrival(arriving_process,
					      running_process);
			running_process = PRIOP_tick(running_process);
			print_and_save_process(running_process, time);

		}
		PRIOP_finish();
		break;

	case RR:
		printf("Starting RR scheduler\n");
		if (RR_startup(RR_quantum) != 0) {
			fprintf(stderr, "Problem starting the RR scheduler\n");
			exit(1);
		}
		for (unsigned int time = 0; time < max_time; time++) {
			//check for new arrival at given point of time
			process *arriving_process =
			    check_new_arrival(process_count, time);

			//Let the RR-scheduler handle the processes
			running_process =
			    RR_new_arrival(arriving_process, running_process);
			running_process = RR_tick(running_process);
			print_and_save_process(running_process, time);
		}
		RR_finish();
		break;

	case SRTN:
		printf("Starting SRTN scheduler\n");
		if (SRTN_startup() != 0) {
			fprintf(stderr,
				"Problem starting the SRTN scheduler\n");
			exit(1);
		}
		for (unsigned int time = 0; time < max_time; time++) {
			//check for new arrival at given point of time
			process *arriving_process =
			    check_new_arrival(process_count, time);

			//Let the SRTN-scheduler handle the processes
			running_process =
			    SRTN_new_arrival(arriving_process, running_process);
			running_process = SRTN_tick(running_process);
			print_and_save_process(running_process, time);
		}
		SRTN_finish();
		break;

	case HRRN:
		printf("Starting HRRN scheduler\n");
		if (HRRN_startup() != 0) {
			fprintf(stderr,
				"Problem starting the HRRN scheduler\n");
			exit(1);
		}
		for (unsigned int time = 0; time < max_time; time++) {
			//check for new arrival at given point of time
			process *arriving_process =
			    check_new_arrival(process_count, time);

			//Let the HRRN-scheduler handle the processes
			running_process =
			    HRRN_new_arrival(arriving_process, running_process);
			running_process = HRRN_tick(running_process);
			print_and_save_process(running_process, time);
		}
		HRRN_finish();
		break;

	case MLF:
		printf("Starting MLF scheduler\n");
		if (MLF_startup() != 0) {
			fprintf(stderr, "Problem starting the MLF scheduler\n");
			exit(1);
		}
		for (unsigned int time = 0; time < max_time; time++) {
			//check for new arrival at given point of time
			process *arriving_process =
			    check_new_arrival(process_count, time);

			//Let the MLF-scheduler handle the processes
			running_process =
			    MLF_new_arrival(arriving_process, running_process);
			running_process = MLF_tick(running_process);
			print_and_save_process(running_process, time);
		}
		MLF_finish();
		break;

	default:
		fprintf(stderr, "Unknown scheduling strategy\n");
		exit(0);
		break;
	}

	free_processes(process_count);
	printf("\n");
	return resulting_schedule;
}
