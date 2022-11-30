#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <wait.h>

#include "banking.h"
#include "phases.h"

void transfer(void *parent_data, local_id src, local_id dst,
              balance_t amount) {
    struct current_proc *current_proc = (struct current_proc *) parent_data;
    TransferOrder transferOrder;
    transferOrder.s_amount = amount;
    transferOrder.s_dst = dst;
    transferOrder.s_src = PARENT_ID;
    Message message;
    memcpy(message.s_payload, &transferOrder, sizeof(transferOrder));
    message.s_header = (MessageHeader) {.s_type = TRANSFER, .s_payload_len = sizeof(transferOrder), .s_magic = MESSAGE_MAGIC, .s_local_time = get_physical_time()};
    //fixme: retry sending
    send(current_proc, src, &message);
    int flag = 0;
    Message receive_msg;
    while (flag == 0) {
        while (receive(current_proc, dst, &receive_msg) == -1) {}
        if (receive_msg.s_header.s_type == ACK) {
            flag = 1; // когда получаем сообщение от получателя баксов, то выходим из цикла
        }
    }
//    // student, please implement me
    // done ;)
}

void close_useless_pipes(struct pipes_house *pipes_house, int local_id, int pipes_count) {
    for (size_t i = 0; i < pipes_count; i++) {
        for (int j = 0; j < pipes_count; j++) {
            if (j == i) {
                continue;
            }
            if (i == local_id) {
                close(pipes_house->house[local_id][j].pipe1.fd[1]); //закрываем те дескриторы, которые не нужны
                close(pipes_house->house[local_id][j].pipe2.fd[0]);
                pipes_house->house[local_id][j].pipe1.fd[1] = -1;
                pipes_house->house[local_id][j].pipe2.fd[0] = -1;
            } else if (j == local_id) {
                close(pipes_house->house[i][local_id].pipe1.fd[0]);
                close(pipes_house->house[i][local_id].pipe2.fd[1]);
                pipes_house->house[i][local_id].pipe1.fd[0] = -1;
                pipes_house->house[i][local_id].pipe2.fd[1] = -1;
            } else {
                close(pipes_house->house[i][j].pipe1.fd[1]);
                close(pipes_house->house[i][j].pipe1.fd[0]);
                close(pipes_house->house[i][j].pipe2.fd[1]);
                close(pipes_house->house[i][j].pipe2.fd[0]);
                pipes_house->house[i][j].pipe1.fd[1] = -1;
                pipes_house->house[i][j].pipe1.fd[0] = -1;
                pipes_house->house[i][j].pipe2.fd[1] = -1;
                pipes_house->house[i][j].pipe2.fd[0] = -1;
            }
        }
    }
}


int main(int argc, char **argv) {

    FILE *file_events = fopen("events.log", "w");
    FILE *file_pipes = fopen("pipes.log", "w");

    int count = strtol(argv[2], NULL, 10); // количество потоков
    if (count < 0 || count > 11) {
        exit(1);
    }

    int state[10];
    for (int i = 0; i < count; i++) {
        int tmp_amount = strtol(argv[3 + i], NULL, 10);
        if (tmp_amount > 0 || tmp_amount < 99) {
            state[i + 1] = tmp_amount;
        }// сумма на счете
    }

    struct pipes_house pipes_house;
    int pipes_count = count + 1; // main учитываем, таким образом + 1, полносвязный граф
    for (size_t i = 0; i < pipes_count; i++) {
        //массив пайпов, например, индекс-строка -- откуда, индекс-столбец -- куда => получаем нужные пайпы
        for (size_t j = i + 1; j < pipes_count; j++) {
            struct my_pipe my_pipe1;
            struct my_pipe my_pipe2;

            pipe(my_pipe1.fd);
            pipe(my_pipe2.fd);
            fcntl(my_pipe1.fd[0], F_SETFL, fcntl(my_pipe1.fd[0], F_GETFL, 0) | O_NONBLOCK);
            fcntl(my_pipe1.fd[1], F_SETFL, fcntl(my_pipe1.fd[1], F_GETFL, 0) | O_NONBLOCK);
            fcntl(my_pipe2.fd[0], F_SETFL, fcntl(my_pipe2.fd[0], F_GETFL, 0) | O_NONBLOCK);
            fcntl(my_pipe2.fd[1], F_SETFL, fcntl(my_pipe2.fd[1], F_GETFL, 0) | O_NONBLOCK);
            struct my_pipe_pair my_pipe_pair = (struct my_pipe_pair) {.pipe1 = my_pipe1, .pipe2 = my_pipe2};
            pipes_house.house[i][j] = my_pipe_pair;
            pipes_house.house[j][i] = (struct my_pipe_pair) {.pipe1 = my_pipe_pair.pipe2, .pipe2 = my_pipe_pair.pipe1};
        }
    }

    struct current_proc current_proc;
    int child = -1;
    int local_id = 0;

    //создаем процессы
    for (int i = 0; i < count; i++) {
        child = fork();

        if (child == 0) { //дочерний процесс
            local_id = i + 1;
            close_useless_pipes(&pipes_house, local_id, pipes_count);
            current_proc = (struct current_proc) {.local_id = local_id, .count = count, .money = state[local_id], .pointer = &pipes_house, .counter = 0};
            current_proc.balance_history.s_id = local_id;
            current_proc.balance_history.s_history_len = 1;
            memset(current_proc.balance_history.s_history, 0, sizeof(current_proc.balance_history.s_history));
            BalanceState balance_state = (BalanceState) {.s_time = get_physical_time(), .s_balance_pending_in = 0, .s_balance = current_proc.money};
            current_proc.balance_history.s_history[0] = balance_state;
            break; // полученный
            // процесс сразу же завершает свое пребывание в цикле
        }
        if (child != 0 && i == count - 1) { //родительский процесс в самый последний момент
            close_useless_pipes(&pipes_house, local_id, pipes_count);
            current_proc = (struct current_proc) {.local_id = 0, .pointer = &pipes_house, .count = count, .money = state[local_id], .counter = 0};
            //bank_robbery()
        }
    }
    pipe_log(file_pipes, &pipes_house, pipes_count);


    //started msg

    first_phase(&current_proc, count, file_events, file_pipes);


    if (local_id == 0) {
        bank_robbery(&current_proc, count);
        Message stop_message;
        stop_message.s_header = (MessageHeader) {.s_type = STOP, .s_payload_len = 0, .s_magic = MESSAGE_MAGIC, .s_local_time = get_physical_time()};
        send_multicast(&current_proc, &stop_message);
    } else {
        second_phase(&current_proc, file_events, file_pipes);
    }

    if(local_id == 0) {
        Message done_msg;
        //int counter = 0;
        for(int i = 1; i < count+1; i++) {
            while (receive(&current_proc, i, &done_msg) == -1) {}
            if (done_msg.s_header.s_type == DONE) {
            }
        }
        receive_done_log(file_events, get_physical_time(), PARENT_ID);
        Message history_msg;
        //counter = 0;
        AllHistory  all_history;
        all_history.s_history_len = 0;
        for(int i = 1; i < count+1; i++) {
            while (receive(&current_proc, i, &history_msg) == -1) {}
            if (history_msg.s_header.s_type == BALANCE_HISTORY) {
                //counter++;
                BalanceHistory current_history;
                all_history.s_history_len++;
                memcpy(&current_history, &(history_msg.s_payload), sizeof(BalanceHistory));
                BalanceState balanceState = current_history.s_history[current_history.s_history_len - 1];
                printf("current money %d \n", balanceState.s_balance);
                printf(" current his len %d \n", current_history.s_history_len);
                printf("curr time %d \n", get_physical_time());
                    if (current_history.s_history_len-1 != get_physical_time()) {
                        int tmp_len = current_history.s_history_len;
                        current_history.s_history_len = get_physical_time()+1;
                        printf("here procc %d \n", current_history.s_id);
                        for(int j = tmp_len; j <= get_physical_time(); j++) {
                            printf("balance %d\n", balanceState.s_balance);
                            current_history.s_history[j] = balanceState;
                            current_history.s_history[j].s_time = j;
                        }
                    }
                printf("parent receive history from %d\n", current_history.s_id);
                printf("history of process %d length %d\n", current_history.s_id, current_history.s_history_len);
                all_history.s_history[current_history.s_id-1] = current_history;
            }
        }
        print_history(&all_history);
    }


//    if (local_id == 0) {
//        bank_robbery(&current_proc, count);
//        Message stop_message;
//        stop_message.s_header = (MessageHeader) {.s_type = STOP, .s_payload_len = 0, .s_magic = MESSAGE_MAGIC, .s_local_time = get_physical_time()};
//        send_multicast(&current_proc, &stop_message);
//
//        //ожидание ответа DONE от участников перевода после сообщения STOP
//        Message done_msg;
//        int counter = 0;
//        while (counter != count) {
//            while (receive_any(&current_proc, &done_msg) == -1) {}
//            if (done_msg.s_header.s_type == DONE) {
//                counter++;
//            }
//        }
//        receive_done_log(file_events, get_physical_time(), PARENT_ID);
//        Message history_msg;
//        counter = 0;
//        AllHistory  all_history;
//        all_history.s_history_len = 0;
//        while (counter != count) {
//            while (receive_any(&current_proc, &history_msg) == -1) {}
//            if (history_msg.s_header.s_type == BALANCE_HISTORY) {
//                counter++;
//                BalanceHistory current_history;
//                all_history.s_history_len++;
//                memcpy(&current_history, &(history_msg.s_payload), sizeof(BalanceHistory));
//                printf("parent receive history from %d\n", current_history.s_id);
//                all_history.s_history[current_history.s_id-1] = current_history;
//            }
//        }
//        print_history(&all_history);
//    } else {
//        second_phase(&current_proc, file_events, file_pipes);
//    }

    while (wait(NULL) > 0);
    return 0;
}

