//
// Created by mashusik on 14.09.22.
//

#include "banking.h"
#include "ipc.h"

#ifndef LAB1_CURRENT_PROC_H
#define LAB1_CURRENT_PROC_H

struct my_pipe {
    int fd[2]; // пайп в одну сторону, т. е. дескриптор откуда, дескр куда
};
struct my_pipe_pair {
    struct my_pipe pipe1;
    struct my_pipe pipe2; // 2 пайпа: 1 - читаем, 2 - пишем
};


struct pipes_house{
    struct my_pipe_pair house [10][10];
};


struct current_proc {
    struct pipes_house * pointer; // указатель на массив всех пайпов
    int count; //количество всех потоков
    local_id counter; // счетчик, указывающий на локал_ид того потока, который отправляет сообщения (для первой фазы)
    local_id local_id;
    int money; // количество денежек
    int transfer_count; // количество потоков, участвующих в банковских переводах
    BalanceHistory balance_history;
};


#endif //LAB1_CURRENT_PROC_H
