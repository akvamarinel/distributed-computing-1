//
// Created by mashusik on 13.09.22.
//
#include "ipc.h"
#include "current_proc.h"
#include <stdio.h>
#include <unistd.h>


int send_multicast(void *self, const Message *msg) {
    struct current_proc *current_proc = (struct current_proc *) self;
    local_id local_id = current_proc->local_id;
    for (int i = 0; i < (current_proc->count) + 1; i++) {
        if (i == local_id) {
            continue;
        } else {
            send(self, i, msg);
        }

    }

    return 0;
}

int send(void *self, local_id dst, const Message *msg) {
    struct current_proc *current_proc = (struct current_proc *) self;
    local_id current_id = current_proc->local_id; // айди отправителя

    struct pipes_house *pipes_house = current_proc->pointer;
    struct my_pipe_pair current_pair = pipes_house->house[current_id][dst];
    int res = write(current_pair.pipe2.fd[1], msg, (msg->s_header.s_payload_len) + sizeof(MessageHeader));
    if (res > 0) {
        printf("process %d write to process %d to fd %d msg with type %d with res %d\n", current_id, dst, current_pair.pipe2.fd[1], msg->s_header.s_type, res);
        fflush(stdout);
    }
    if(res >= 0) {
        return 0;
    } else {
        return -1;
    }
}

int receive(void *self, local_id from, Message *msg) {
    struct current_proc *current_proc = (struct current_proc *) self;
    local_id current_id = current_proc->local_id; // айди получателя
    if (current_id == from) {
        return -1;
    }
    struct pipes_house *pipes_house = current_proc->pointer;
    struct my_pipe_pair current_pair = pipes_house->house[current_id][from];

    int tmp = read(current_pair.pipe1.fd[0], &(msg->s_header), sizeof(MessageHeader));
    if (tmp == -1) return -1;
    int tmp2 = read(current_pair.pipe1.fd[0], msg->s_payload, msg->s_header.s_payload_len);
    if ((tmp + tmp2) > 0) {
        printf("process %d read from process %d from fd %d msg with type %d with res %d\n", current_id, from, current_pair.pipe1.fd[0], msg->s_header.s_type, tmp + tmp2);
        fflush(stdout);
    }
    if (tmp == -1 || tmp2 == -1) {
        return -1;
    }

    return 0;
}

int receive_any(void *self, Message *msg) {
    struct current_proc *current_proc = (struct current_proc *) self;
    local_id local_id = current_proc->local_id;
    while(1) {
        for (int i = 0; i < (current_proc->count) + 1; i++) {
            if (i == local_id) {
                continue;
            } else {
                int res = receive(self, i, msg);
                if (res >= 0) {
                    return 0;
                }
            }
        }
    }

    return -1;
}





