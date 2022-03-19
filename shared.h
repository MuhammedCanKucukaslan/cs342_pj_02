#include <mqueue.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
enum state
{
    RUNNING,
    READY,
    WAITING
};
enum algorithm
{
    FCFS,
    SJF,
    RR
};
enum burst_distribution
{
    UNI,
    EXP,
    FIX
};
enum task_type
{
    TERMINATE,
    IO1,
    IO2
};
/*
#define FCFS "FCFS"
#define SJF "SJF"
#define RR  "RR"
*/
struct PCB {
    // pid (a positive integer)
    // thread id (of type pthread_t),
    // state,
    // next CPU burst length (ms),
    // remaining CPU burst length to execute (ms) (for RR scheduling),
    // number of CPU bursts executed so far,
    // time spent in ready list so far (ms),
    // number of times I/O is done with device1,
    // number of times I/O is done with device2,
    // start time (ms), i.e., arrival time,
    // finish time (ms),
    // total execution time in CPU so far
    int pid;
    pthread_t tid;
    enum state state;
    int next_burst_length;
    int remaining_burst_length;
    int num_bursts;
    int time_in_ready_list;
    int IO_device1;
    int IO_device2;
    
    int start_time;
    int finish_time;
    int total_execution_time;
};
struct pcb_node {
    struct PCB pcb;
    struct pcb_node *next;
};
struct pcb_queue {
    struct pcb_node *head;
    struct pcb_node *tail;
    int count;
};
/* DEPRECATED
void pcb_to_pcbptr(struct PCB pcb, struct PCB *pcbptr)
{
    // copy the pcb data to the pcbptr
    pcbptr->pid = pcb.pid;
    pcbptr->tid = pcb.tid;
    pcbptr->state = pcb.state;
    pcbptr->next_burst_length = pcb.next_burst_length;
    pcbptr->remaining_burst_length = pcb.remaining_burst_length;
    pcbptr->num_bursts = pcb.num_bursts;
    pcbptr->time_in_ready_list = pcb.time_in_ready_list;
    pcbptr->IO_device1 = pcb.IO_device1;
    pcbptr->IO_device2 = pcb.IO_device2;
    pcbptr->start_time = pcb.start_time;
    pcbptr->finish_time = pcb.finish_time;
    pcbptr->total_execution_time = pcb.total_execution_time;


}
*/