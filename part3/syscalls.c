#include <linux/syscalls.h>
#include <linux/kernel.h>

// Elevator tracker variables

typedef enum {
    IDLE,
    MOVING,
    LOADING,
    OFFLINE
} ElevatorState;

static ElevatorState elevator_state = OFFLINE;
static int current_floor = 1;
static int num_passengers = 0;

SYSCALL_DEFINE0(start_elevator)
{
    if (elevator_state != OFFLINE) {
        printk(KERN_INFO "Elevator is already active.");
        return 1;
    }
    
    // Initialize elevator state, floor, and passengers
    elevator_state = IDLE;
    current_floor = 1;
    num_passengers = 0;

    // Any additional initialization if needed

    printk(KERN_INFO "Elevator started.");
    return 0;
}

SYSCALL_DEFINE3(issue_request, int, start_floor, int, destination_floor, int, type)
{
    if (start_floor < 1 || start_floor > 6 || destination_floor < 1 || destination_floor > 6 || type < 0 || type > 3) {
        printk(KERN_INFO "Invalid request.");
        return 1;
    }

    // Handle request and place the student on the appropriate floor's queue.
    // Put elevator module logic here

    printk(KERN_INFO "Request issued: Start: %d, Destination: %d, Type: %d", start_floor, destination_floor, type);
    return 0;
}

SYSCALL_DEFINE0(stop_elevator)
{
    if (elevator_state == OFFLINE) {
        printk(KERN_INFO "Elevator is already offline.");
        return 1;
    }

    // Check if elevator is empty
    if (num_passengers > 0) {
        printk(KERN_INFO "Elevator has passengers. Can't stop now.");
        return -1; // or appropriate error number
    }

    // Stop elevator
    elevator_state = OFFLINE;
    printk(KERN_INFO "Elevator stopped.");
    return 0;
}


// COMMENTS FOR OUR FUTURE REFERENCE:
//  kernel version directory name: 5.4.0-162-generic

//  for syscall_64.tbl:
// 548     64      start_elevator          sys_start_elevator
// 549     64      issue_request            sys_issue_request
// 550     64      stop_elevator            sys_stop_elevator

// for syscalls.h
// asmlinkage int sys_start_elevator(void);
// asmlinkage int sys_issue_request(int start_floor, int destination_floor, int type);
// asmlinkage int sys_stop_elevator(void);


// for Makefile:
// core-y          := syscalls/