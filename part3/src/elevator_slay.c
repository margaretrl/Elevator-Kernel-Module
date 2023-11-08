#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/linkage.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("Dorm elevator simulation");

#define ENTRY_NAME "elevator"
#define PERMS 0644
#define PARENT NULL //What is this?

static struct proc_dir_entry* elevator_entry;
static struct task_struct *elevator_thread;

// H attempt added stuff

//Maybe we should move these stubs to init function
// I think these functions are like how you relate it to the syscalls
extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int, int, int);
extern int (*STUB_stop_elevator)(void);

#define MAX_PASSENGERS 5
#define MAX_WEIGHT 750

#define PASS_FRESHMAN 0
#define PASS_SOPHMORE 1
#define PASS_JUNIOR 2
#define PASS_SENIOR 3
#define WEIGHT_FRESHMAN 100
#define WEIGHT_SOPHMORE 150
#define WEIGHT_JUNIOR 200
#define WEIGHT_SENIOR 250
#define MIN_FLOOR 1
#define MAX_FLOOR 6


// mutex thing idk
static DEFINE_MUTEX(elevator_mutex);
// like idek anymore
DECLARE_WAIT_QUEUE_HEAD(elevator_wait_queue);

static int pending_requests = 0; // ppl waiting

// struct {
//     int total_cnt;
//     int total_weight;
//     struct list_head list;
// } passengers;

typedef enum{
    ELEVATOR_OFFLINE,
    ELEVATOR_IDLE,
    ELEVATOR_LOADING,
    ELEVATOR_UP,
    ELEVATOR_DOWN
} elevator_state;


typedef struct passenger {
    int id;
    int weight;
    int start_floor;
    int dest_floor;
    char s_type; //student type
    struct list_head list;
} Passenger;

struct elevator {
    elevator_state state;
    int current_floor;
    int target_floor;
    int weight;
    int status;
    int ppl_on_board;
    struct list_head passengers;
    struct task_struct *elevator_thread; //our kthread
    struct mutex lock;
} my_elevator;

struct list_head building[7]; // arr of lists for floors
struct mutex building_lock;

// POSSIBLY WE WILL USE THESE VARS

int stop;
static struct proc_dir_entry* proc_entry;
int ppl_waiting_on_floor[7];
int ppl_serviced = 0; //initialize to 0


// Function declarations
int elevator_thread_function(void *data);
void add_passenger2(void);
void del_passenger2(void);



int add_passenger(int type)
{
    int weight;
    char s_type;
    Passenger *p;

    if (my_elevator.ppl_on_board >= MAX_PASSENGERS)
        return 0;

// switch on what is inputted into add passenger and its gonna get called in elevator_proc_open i think
    switch (type)
{
    case PASS_FRESHMAN:
        weight = 100;
        s_type = "F";
        break;
    case PASS_SOPHMORE:
        weight = 150;
        s_type = "O";
        break;
    case PASS_JUNIOR:
        weight = 200;
        s_type = "J";
        break;
    case PASS_SENIOR:
        weight = 250;
        s_type = "S";
        break;
    default:
        return -1;
}

    p = kmalloc(sizeof(Passenger) * 1, __GFP_RECLAIM);
    if (p == NULL)
        return -ENOMEM;

    p->id = type;
    p->weight = weight;
    p->s_type = s_type;

    // added to back cuz FIFO
    list_add_tail(&p->list, &my_elevator.passengers);


    my_elevator.ppl_on_board += 1;

    my_elevator.weight += weight;

    return 0;
}

int del_passenger(int type)
{


    return 0;
}

int elevator_thread_function(void *data) {
    // This thread will run until it's told to stop
    while (!kthread_should_stop()) {
        // Lock the mutex before accessing/changing shared data
        // Wait for a condition to be true before proceeding
        wait_event_interruptible(elevator_wait_queue, (pending_requests > 0) || (my_elevator.state == ELEVATOR_OFFLINE));

        if (kthread_should_stop())
            break;
        mutex_lock(&elevator_mutex);
        if (my_elevator.state != ELEVATOR_OFFLINE)
        {

        }

        /*
        // Here you will check the state of the elevator and decide what to do next.
        // For instance, if the elevator is not on the target floor, it should move towards it.
        if (my_elevator.current_floor != my_elevator.target_floor) {
            // Elevator logic to move to the next floor
            if (my_elevator.current_floor < my_elevator.target_floor) {
                my_elevator.current_floor++;
            } else {
                my_elevator.current_floor--;
            }

            // Simulate the time taken to move a floor
            mutex_unlock(&elevator_mutex);
            ssleep(2);
            mutex_lock(&elevator_mutex);
        } else {
            // Elevator has reached the target floor
            // Logic to load or unload passengers

            // Simulate the time taken to load/unload passengers
            mutex_unlock(&elevator_mutex);
            ssleep(1);
            mutex_lock(&elevator_mutex);

            // Determine the next target floor based on the requests
            // For now, let's just set a dummy floor - this is where your scheduling logic will go
            my_elevator.target_floor = (my_elevator.target_floor % 6) + 1;
        }*/

        // After processing, release the mutex
        mutex_unlock(&elevator_mutex);

        // Sleep for a bit before the next iteration to not hog the CPU
        msleep(2);
    }
    return 0;
}

const char* get_elevator_state_string(elevator_state state) {
    switch (state) {
        case ELEVATOR_OFFLINE:
            return "Offline";
        case ELEVATOR_IDLE:
            return "Idle";
        case ELEVATOR_LOADING:
            return "Loading";
        case ELEVATOR_UP:
            return "Moving Up";
        case ELEVATOR_DOWN:
            return "Moving Down";
        default:
            return "Unknown";
    }
}
// H attempt added stuff end

// static ssize_t elevator_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
// {
//     //char buf[10000];
//     char *buf = kmalloc(sizeof(char)*800,__GFP_RECLAIM);
//     int len = 0;
//     // New stuff
//     int total_waiting = 0

//     for (int i = 1; i < 7; i++){
//         total_waiting+=ppl_waiting_on_floor[i];
//     }

//     if(mutex_lock_interruptible(&Elevator.mutex) == 0)
//     {
//     len = sprintf(buf, "Elevator state: %s\n", get_elevator_state_string(my_elevator.state));
//     len += sprintf(buf + len, "Current floor: \n", my_elevator.current_floor);
//     len += sprintf(buf + len, "Current load: \n", my_elevator.weight, " lbs");
//     len += sprintf(buf + len, "Elevator status: \n");

//     // Printing out floor structure
//     static char *printstr;
//     strcat(printstr,buf);

//     for (int i = 7; i > 0; i--){
//         strcat(printstr,"[");
//         if(i == Elevator.current_floor) {
//             strcat(printstr,"*");
//         }
//         else {
//             strcat(printstr, " ");
//         }
//         sprintf(buf,"] Floor %d: %d ", i, ppl_waiting_on_floor[i]);
//         strcat(printstr, buf);
//     }

//     list_for_each(temp, &building[i]) {
//         p = list_entry(temp, Passenger, passengerList);
//         strcat(printstr,p->s_type" ");
//     }
//     //strcat(printstr,"\n");

    
//     // len += sprintf(buf + len, "\n[ ] Floor 6: \n");
//     // len += sprintf(buf + len, "[ ] Floor 5: \n");
//     // len += sprintf(buf + len, "[ ] Floor 4: \n");
//     // len += sprintf(buf + len, "[ ] Floor 3: \n");
//     // len += sprintf(buf + len, "[ ] Floor 2: \n");
//     // len += sprintf(buf + len, "[ ] Floor 1: \n");

//     len += sprintf(buf + len, "\nNumber of passengers: %d\n", passengers.total_cnt);
//     len += sprintf(buf + len, "Number of passengers waiting: \n", pending_requests);
//     len += sprintf(buf + len, "Number of passengers serviced: \n", ppl_serviced);

//     kfree(buf);
//     return simple_read_from_buffer(ubuf, count, ppos, buf, len); // better than copy_from_user
// }

static ssize_t elevator_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    char *buf = kmalloc(sizeof(char) * 800, __GFP_RECLAIM);
    int len = 0;
    struct list_head *temp;
    Passenger *p;

    // Check if buf allocation was successful
    if (buf == NULL) {
        return -ENOMEM; // Return an error code indicating memory allocation failure
    }

    // New stuff
    int total_waiting = 0;

    for (int i = 1; i < 7; i++) {
        total_waiting += ppl_waiting_on_floor[i];
    }

    if (mutex_lock_interruptible(&my_elevator.lock) == 0) {
        len = sprintf(buf, "Elevator state: %s\n", get_elevator_state_string(my_elevator.state));
        len += sprintf(buf + len, "Current floor: %d\n", my_elevator.current_floor);
        len += sprintf(buf + len, "Current load: %d lbs\n", my_elevator.weight);

        // Printing out floor structure
        for (int i = 7; i > 0; i--) {
            char* floor_marker;
            if (i == my_elevator.current_floor) {
                floor_marker = "*";
            } else {
                floor_marker = " ";
            }
            len += sprintf(buf + len, "[%s] Floor %d: %d ", floor_marker, i, ppl_waiting_on_floor[i]);
            list_for_each(temp, &building[i]) {
            p = list_entry(temp, Passenger, list);
            len += sprintf(buf + len, "%s ", p->s_type);
            }
        }


        

        len += sprintf(buf + len, "\nNumber of passengers: %d\n", my_elevator.ppl_on_board);
        len += sprintf(buf + len, "Number of passengers waiting: %d\n", pending_requests);
        len += sprintf(buf + len, "Number of passengers serviced: %d\n", ppl_serviced);

        mutex_unlock(&my_elevator.lock);

        // Copy constructed string to user space
        ssize_t result = simple_read_from_buffer(ubuf, count, ppos, buf, len);

        // Free allocated memory for buf
        kfree(buf);

        return result;
    } else {
        // Mutex lock failed
        kfree(buf);
        return -EINTR;
    }
}

static const struct proc_ops elevator_fops = {
    .proc_read = elevator_read,
    //.proc_open = elevator_open,
    //.proc_release = elevator_release,
};


int start_elevator_funct(void) {
    int err;

    // Protect the elevator operations with a mutex
    mutex_lock(&elevator_mutex);

    // Check if the elevator is already running
    if (my_elevator.state != ELEVATOR_OFFLINE) {
        printk(KERN_NOTICE "Elevator is already started.\n");
        mutex_unlock(&elevator_mutex);
        return -EBUSY; // Elevator is already started
    }

    if (elevator_thread != NULL) {
        printk(KERN_NOTICE "Elevator thread already exists.\n");
        mutex_unlock(&elevator_mutex);
        return -EBUSY; // Thread is already created
    }
    // Start the elevator thread

    elevator_thread = kthread_run(elevator_thread_function, NULL, "elevator_thread");
    if (IS_ERR(elevator_thread)) {
        err = PTR_ERR(elevator_thread);
        elevator_thread = NULL;
        printk(KERN_ALERT "Failed to start elevator thread: %d\n", err);
        mutex_unlock(&elevator_mutex);
        return err;
    }
    my_elevator.state = ELEVATOR_IDLE;
    printk(KERN_NOTICE "Elevator started successfully.\n");
    mutex_unlock(&elevator_mutex);
    return 0;
}

int issue_request_funct(int start_floor, int destination_floor, int type) {
    // handle request now
    mutex_lock(&elevator_mutex);
    pending_requests++;
    wake_up_interruptible(&elevator_wait_queue);
    mutex_unlock(&elevator_mutex);
    return 0;
}

int stop_elevator_funct(void) {
    // Protect the elevator operations with a mutex
    mutex_lock(&elevator_mutex);
    wake_up_interruptible(&elevator_wait_queue);
    // Check if the elevator is already running
    if ((my_elevator.state == ELEVATOR_OFFLINE) || !elevator_thread)
    {
        printk(KERN_INFO "Elevator is not running.\n");
        return -EINVAL; // Elevator is not started
    }


    if ((my_elevator.state == ELEVATOR_UP) || (my_elevator.state == ELEVATOR_DOWN)) {
        printk(KERN_NOTICE "Elevator is busy.\n");
        mutex_unlock(&elevator_mutex);
        return -EBUSY; // Elevator is already started
    }

    // Signal the elevator thread to stop
    if (elevator_thread) {
        printk(KERN_NOTICE "Stopping the elevator thread.\n");
        // Signal your thread to stop using a shared flag or condition

        // Wake up the thread if it is sleeping
        // ...

        // Wait for the thread to finish
        printk(KERN_NOTICE "Waiting for the elevator thread to finish.\n");
        kthread_stop(elevator_thread);
        elevator_thread = NULL;
    }


    // Set the elevator state to stopped

    my_elevator.state = ELEVATOR_OFFLINE;
    pending_requests = 0;
    printk(KERN_NOTICE "Elevator stopped.\n");
    mutex_unlock(&elevator_mutex);

    // Clean up or reset your elevator state and request data structures here
    // ...

    return 0;
}


static int __init elevator_init(void)
{
    STUB_start_elevator = start_elevator_funct;
    STUB_issue_request = issue_request_funct;
    STUB_stop_elevator = stop_elevator_funct;

    // Initialize the elevator state
    mutex_lock(&elevator_mutex);
    my_elevator.state = ELEVATOR_OFFLINE;
    my_elevator.current_floor = 1;
    my_elevator.target_floor = 1;
    my_elevator.weight = 0;
    mutex_unlock(&elevator_mutex);

    // create proc entry
    elevator_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &elevator_fops);
    if (!elevator_entry) {
        return -ENOMEM;
    }
    /*
    // Start the elevator thread
    elevator_thread = kthread_run(elevator_thread_function, NULL, "elevator_thread");
    if (IS_ERR(elevator_thread)) {
        // Handle error
        remove_proc_entry(ENTRY_NAME, NULL);
        return PTR_ERR(elevator_thread);
    }
*/
    return 0;
}

static void __exit elevator_exit(void)
{
    // I think need to do like if waiting stuff
    printk("elevator exit is being run");
    stop_elevator_funct();
    if (elevator_entry)
    {
        proc_remove(elevator_entry);
        elevator_entry = NULL;
    }
    STUB_start_elevator = NULL;
    STUB_issue_request = NULL;
    STUB_stop_elevator = NULL;
    mutex_destroy(&elevator_mutex);
}

module_init(elevator_init);
module_exit(elevator_exit);
