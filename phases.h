//
// Created by mashusik on 13.11.22.
//

#ifndef NEWLAB2_PHASES_H
#define NEWLAB2_PHASES_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>


#include "ipc.h"
#include "current_proc.h"
#include "logger.h"


void first_phase(void *self, int count_process, FILE *file_events, FILE *file_pipes);
void first_phase_child(void *self, int count_process, FILE *file_events);
void first_phase_parent(void *self, int count_process, FILE *file_events);
void third_phase(void *self, FILE *file_events, FILE *file_pipes);


void second_phase(void *self, FILE *file_events, FILE *file_pipes);



#endif //NEWLAB2_PHASES_H
