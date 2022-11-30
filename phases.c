//
// Created by mashusik on 13.11.22.
//
#include "phases.h"
#include <assert.h>
int receive_any_child(void *self, Message *msg) {
    struct current_proc *current_proc = (struct current_proc *) self;
    local_id local_id = current_proc->local_id;
    for (int i = 1; i < (current_proc->count) + 1; i++) {
        if (i == local_id) {
            continue;
        } else {
            int res = receive(self, i, msg);
            if (res >= 0) {
                return 0;
            }
        }
    }

    return -1;
}

void first_phase(void *self, int count_process, FILE *file_events, FILE *file_pipes) {
    struct current_proc *current_proc = (struct current_proc *) self;
    local_id current_id = current_proc->local_id; // айди отправителя
    if (current_id == 0) {
        first_phase_parent(self, count_process, file_events);
    } else {
        first_phase_child(self, count_process, file_events);
    }
}

void first_phase_parent(void *self, int count_process, FILE *file_events) {
    Message receive_message;
    int start_msg_counter = 0;
    while (start_msg_counter != count_process) {
        while (receive_any(self, &receive_message) == -1) {}
        if (receive_message.s_header.s_type == STARTED) {
            start_msg_counter++;
        }
    }
    receive_start_log(file_events, get_physical_time(), PARENT_ID);

}

void first_phase_child(void *self, int count_process, FILE *file_events) {
    struct current_proc *current_proc = (struct current_proc *) self;
    local_id current_id = current_proc->local_id; // айди отправителя
    Message message;
    sprintf(message.s_payload, log_started_fmt, get_physical_time(), current_id, getpid(), getppid(),
            current_proc->money);
    message.s_header = (MessageHeader) {.s_type = STARTED, .s_payload_len = strlen(
            message.s_payload), .s_magic = MESSAGE_MAGIC, .s_local_time = get_physical_time()};
    send_multicast(self, &message);
    start_log(file_events, get_physical_time(), current_id, getpid(), getppid(), current_proc->money);
    Message receive_message;
    int start_msg_counter = 1;
    while (start_msg_counter < count_process + 1) {
        if (start_msg_counter == current_id) {
            start_msg_counter++;
            if (start_msg_counter == count_process + 1) {
                break;
            }
        }
        while (receive(self, start_msg_counter, &receive_message) == -1) {}
        printf("TYPE of message %d first phase\n", message.s_header.s_type);
        if (receive_message.s_header.s_type == STARTED) {
            start_msg_counter++;
        }
    }
    receive_start_log(file_events, get_physical_time(), current_id);
}


void second_phase(void *self, FILE *file_events, FILE *file_pipes) {
    struct current_proc *current_proc = (struct current_proc *) self;
    local_id current_id = current_proc->local_id;
    int flag = 0;
    int done_counter = 0;
    int have_stop = 0;
    while (flag == 0) {
        Message message;
        while (receive_any(current_proc, &message) == -1) {}
        printf("TYPE of message %d\n", message.s_header.s_type);
        switch (message.s_header.s_type) {
            case STOP: {
                Message new_message;
                new_message.s_header = (MessageHeader) {.s_type = DONE, .s_payload_len = 0, .s_magic = MESSAGE_MAGIC, .s_local_time = 0};
                send_multicast(current_proc, &new_message);
                done_log(file_events, get_physical_time(), current_id, current_proc->money);
                //Message done_message;
                have_stop = 1;

                if (have_stop == 1 && done_counter == current_proc->count - 1) {
                    flag = 1;
                    Message history_message;
                //    BalanceState balanceState = current_proc->balance_history.s_history[current_proc->balance_history.s_history_len - 1];
                    current_proc->balance_history.s_history_len = get_physical_time()+1;
//                    if (current_proc->balance_history.s_history_len-1 != get_physical_time()) {
//                        for(int j = current_proc->balance_history.s_history_len; j <= get_physical_time(); j++) {
//                            current_proc->balance_history.s_history[j] = balanceState;
//                            current_proc->balance_history.s_history[j].s_time = j;
//                        }
//                    }
                    size_t all_size = sizeof(current_proc->balance_history);
                    size_t useful_part = sizeof(BalanceState) * current_proc->balance_history.s_history_len;
                    size_t result_size = all_size - (sizeof(current_proc->balance_history.s_history) - useful_part);
                    memcpy(history_message.s_payload, &(current_proc->balance_history), result_size);
                    history_message.s_header = (MessageHeader) {.s_type = BALANCE_HISTORY, .s_payload_len = result_size, .s_magic = MESSAGE_MAGIC, .s_local_time = get_physical_time()};
                    send(current_proc, PARENT_ID, &history_message);
                }
                break;
            }
            case TRANSFER: {
                TransferOrder *transferOrder = (TransferOrder *) (&(message.s_payload[0]));
                int s_src = transferOrder->s_src; //если от K процесса, то пересылаем и вычитаем у себя $$
                if (s_src == PARENT_ID) {
                    timestamp_t tmp_time = get_physical_time();
                    current_proc->money = current_proc->money - transferOrder->s_amount;
                    int tmp_len = current_proc->balance_history.s_history_len;
                    assert(tmp_time > tmp_len - 1);
                    current_proc->balance_history.s_history_len = tmp_time + 1;
                    current_proc->balance_history.s_history[tmp_time] = (BalanceState) {.s_balance = current_proc->money, .s_time = tmp_time, .s_balance_pending_in = 0};
                    BalanceState balanceState = current_proc->balance_history.s_history[tmp_len - 1];
                    printf("длина истории %d в процессе %d\n", balanceState.s_balance, current_id);

                    for (int i = tmp_len; i < tmp_time; i++) {
                        current_proc->balance_history.s_history[i] = balanceState;
                        current_proc->balance_history.s_history[i].s_time = i;
                    }
                    TransferOrder transfer_order;
                    transfer_order.s_amount = transferOrder->s_amount;
                    transfer_order.s_dst = transferOrder->s_dst;
                    transfer_order.s_src = current_id;
                    Message trnsf_message;
                    memcpy(trnsf_message.s_payload, &transfer_order, sizeof(transfer_order));
                    trnsf_message.s_header = (MessageHeader) {.s_type = TRANSFER, .s_payload_len = sizeof(transfer_order), .s_magic = MESSAGE_MAGIC, .s_local_time = get_physical_time()};
                    send(current_proc, transfer_order.s_dst, &trnsf_message);
                    receive_transfer_log_out(file_events, get_physical_time(), current_id, transfer_order.s_amount,
                                             transfer_order.s_dst);
                    break;
                } else {
                    printf("process %d receive message from %d and now will send ack\n", current_id, s_src);
                    timestamp_t tmp_time = get_physical_time();
                    current_proc->money = current_proc->money + transferOrder->s_amount;
                    int tmp_len = current_proc->balance_history.s_history_len;
                    assert(tmp_time > tmp_len - 1);
                    current_proc->balance_history.s_history_len = tmp_time + 1;
                    current_proc->balance_history.s_history[tmp_time] = (BalanceState) {.s_balance = current_proc->money, .s_time = tmp_time, .s_balance_pending_in = 0};
                    BalanceState balanceState = current_proc->balance_history.s_history[tmp_len - 1];
                    printf("история  %d в процессе %d\n", balanceState.s_balance, current_id);

                    for (int i = tmp_len; i < tmp_time; i++) {
                        current_proc->balance_history.s_history[i] = balanceState;
                        current_proc->balance_history.s_history[i].s_time = i;
                    }
                    receive_transfer_log_in(file_events, get_physical_time(), current_id,
                                            transferOrder->s_amount, transferOrder->s_src);
                    Message ACK_message;
                    ACK_message.s_header = (MessageHeader) {.s_payload_len = 0, .s_type = ACK, .s_local_time = get_physical_time(), .s_magic = MESSAGE_MAGIC};
                    send(current_proc, PARENT_ID, &ACK_message);
                }
                break;
            }
            case DONE: {
                done_counter++;
                if (have_stop == 1 && done_counter == current_proc->count - 1) {
                    Message history_message;
                    size_t all_size = sizeof(current_proc->balance_history);
                    size_t useful_part = sizeof(BalanceState) * current_proc->balance_history.s_history_len;
                    size_t result_size = all_size - (sizeof(current_proc->balance_history.s_history) - useful_part);
                    memcpy(history_message.s_payload, &(current_proc->balance_history), result_size);
                    history_message.s_header = (MessageHeader) {.s_type = BALANCE_HISTORY, .s_payload_len = result_size, .s_magic = MESSAGE_MAGIC, .s_local_time = get_physical_time()};
                    send(current_proc, PARENT_ID, &history_message);
                    flag = 1;
                }
                break;
            }
            default: {
                printf("Unknown message type: %d\n", message.s_header.s_type);
                break;
            }
        }
    }
}

//void third_phase(void *self, FILE *file_events, FILE *file_pipes) {
//    struct current_proc *current_proc = (struct current_proc *) self;
//    local_id current_id = current_proc->local_id;
//    Message new_message;
//    new_message.s_header = (MessageHeader) {.s_type = DONE, .s_payload_len = 0, .s_magic = MESSAGE_MAGIC, .s_local_time = 0};
//    send_multicast(current_proc, &new_message);
//    done_log(file_events, get_physical_time(), current_id, current_proc->money);
//    //Message done_message;
//    // int counter = 0;
//    for (int i = 1; i < current_proc->count + 1; i++) {
//        if (i == current_id) {
//            continue;
//        }
//        int flag = 0;
//        while (flag == 0) {
//            while (receive(self, i, &done_message) != -1) {}
//            if (done_message.s_header.s_type == DONE) {
//                printf("process %d receive from process %d DONE msg\n", current_id, i);
//                flag = 1;
//            }
//        }
//    }
//
//}
