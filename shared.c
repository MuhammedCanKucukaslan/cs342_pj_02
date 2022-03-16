enum state {RUNNING, READY, WAITING};
enum algorithm {FCFS, SJF, RR};
enum burst_distribution {UNI, EXP, FIX};
/*
#define FCFS "FCFS"
#define SJF "SJF"
#define RR  "RR"
*/
struct PCB {
    //pid (a positive integer)
    //thread id (of type pthread_t), 
    //state, 
    //next CPU burst length (ms), 
    //remaining CPU burst length to execute (ms) (for RR scheduling),
    //number of CPU bursts executed so far, 
    //time spent in ready list so far (ms), 
    //number of times I/O is done with device1,
    //number of times I/O is done with device2,
    //start time (ms), i.e., arrival time, 
    //finish time (ms),
    //total execution time in CPU so far
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