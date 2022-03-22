#include <mqueue.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
enum BOOL
{
    FALSE,
    TRUE
};
enum state
{
    RUNNING,
    READY,
    WAITING,
    TERMINATED
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

/*
#define FCFS "FCFS"
#define SJF "SJF"
#define RR  "RR"
*/
struct PCB {

    long pid;                  // pid (a positive integer)
    pthread_t tid;             // thread id (of type pthread_t),
    enum state state;          // state,
    int next_burst_length;     // next CPU burst length (ms),
    int remaining_burst_length;// remaining CPU burst length to execute (ms) (for RR scheduling),
    int num_bursts;            // number of CPU bursts executed so far,
    int time_in_ready_list;    // time spent in ready list so far (ms),
    int IO_device1;            // number of times I/O is done with device1,
    int IO_device2;            // number of times I/O is done with device2,
    long start_time;           // start time (ms), i.e., arrival time,
    int finish_time;           // finish time (ms),
    int total_execution_time;  // total execution time in CPU so far
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