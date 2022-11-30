//
// Created by mashusik on 26.09.22.
//

#ifndef LAB1_LOGGER_H
#define LAB1_LOGGER_H

#include <stdio.h>
#include "common.h"
#include "ipc.h"
#include "pa2345.h"
#include "current_proc.h"

void start_log(FILE * file, timestamp_t time, local_id id, int pid, int ppid, int amount);

void receive_start_log(FILE * file, timestamp_t time, local_id id);

void done_log(FILE * file, timestamp_t time, local_id id, int money);

void receive_done_log(FILE * file, timestamp_t time, local_id id);

void pipe_log(FILE * file, struct pipes_house * pipes_house, int count);

void transfer_log(FILE * file, local_id id_from, local_id id_in);

void receive_transfer_log_out(FILE * file, timestamp_t time, local_id id_from, int amount, local_id id_in);

void receive_transfer_log_in(FILE * file,timestamp_t time, local_id id_from, int amount, local_id id_in);


#endif //LAB1_LOGGER_H
