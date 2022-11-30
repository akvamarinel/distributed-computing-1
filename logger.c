//
// Created by mashusik on 26.09.22.
//

#include <stdio.h>
#include "logger.h"

//"Process %1d (pid %5d, parent %5d) has STARTED\n";
void start_log(FILE * file, timestamp_t time, local_id id, int pid, int ppid, int amount) {
    printf(log_started_fmt,time, id, pid, ppid, amount);
    fprintf(file, log_started_fmt, time, id, pid, ppid, amount);
    fflush(file);
    fflush(stdout);
}


//"Process %1d received all STARTED messages\n";
void receive_start_log(FILE * file, timestamp_t time, local_id id){
    printf(log_received_all_started_fmt, time, id);
    fprintf(file, log_received_all_started_fmt, time, id);
    fflush(file);
    fflush(stdout);
}

// "Process %1d has DONE its work\n";
void done_log(FILE * file,timestamp_t time, local_id id, int money) {
    printf(log_done_fmt, time, id, money);
    fprintf(file, log_done_fmt, time, id, money);
    fflush(file);
    fflush(stdout);
}

void receive_done_log(FILE * file, timestamp_t time, local_id id) {
    printf(log_received_all_done_fmt, time,  id);
    fprintf(file, log_received_all_done_fmt, time, id);
    fflush(file);
    fflush(stdout);
}

void pipe_log(FILE * file, struct pipes_house * pipes_house, int count){
    static const char * const pipes_log_fmt =
            "Process %d has pipes: %3d and %3d\n";
    for (int i = 0; i < count; i++) {
        for(size_t j = 0; j < count; j++) {
            if(i == j) {
                continue;
            }
            fprintf(file,pipes_log_fmt, i, pipes_house->house[i][j].pipe1.fd[0], pipes_house->house[i][j].pipe2.fd[1]);
        }

    }
    fflush(file);
    fflush(stdout);
}

void receive_transfer_log_out(FILE * file, timestamp_t time, local_id id_from, int amount, local_id id_in) {
    printf(log_transfer_out_fmt, time, id_from, amount, id_in);
    fprintf(file, log_transfer_out_fmt, time, id_from, amount, id_in);
    fflush(file);
    fflush(stdout);
}

void receive_transfer_log_in(FILE * file, timestamp_t time, local_id id_in, int amount, local_id id_from) {
    printf(log_transfer_in_fmt, time, id_in, amount, id_from);
    fprintf(file, log_transfer_in_fmt, time, id_in, amount, id_from);
    fflush(file);
    fflush(stdout);

}



