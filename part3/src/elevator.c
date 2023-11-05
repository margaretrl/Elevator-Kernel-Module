#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/delay.h>



MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("Example of kernel module proc file for elevator");

#define ENTRY_NAME "elevator"
#define PERMS 0644
#define PARENT NULL

static struct proc_dir_entry* elevator_entry;
static struct task_struct *elevator_thread;

// H attempt added stuff

#define  MAX_PASSENGERS 5
#define MAX_WEIGHT 750

#define PASS_FRESHMAN 0
#define PASS_SOPHMORE 1
#define PASS_JUNIOR 2
#define PASS_SENIOR 3


struct {
    int total_cnt;
    int total_weight;
    struct list_head list;
} passengers;

typedef struct passenger {
    int id;
    int weight;
    int start_floor;
    int dest_floor;
    const char *name;
    struct list_head list;
} Passenger;

struct elevator {
    int state;
    int current_floor;
    int target_floor;
    int weight;
    struct list_head passengers;
    struct task_struct *elevator_thread;
    struct mutex lock;
} my_elevator;

int add_passenger(int type)
{
    int weight;
    char *name;
    Passenger *p;

    if (passengers.total_cnt >= MAX_PASSENGERS)
        return 0;

// switch on what is inputted into add passenger and its gonna get called in elevator_proc_open i think
    switch (type)
{
    case PASS_FRESHMAN:
        weight = 100;
        // dont know if i need this name part
        name = "Freshman";
        break;
    case PASS_SOPHMORE:
        weight = 150;
        name = "Sophmore";
        break;
    case PASS_JUNIOR:
        weight = 200;
        name = "Junior";
        break;
    case PASS_SENIOR:
        weight = 250;
        name = "Senior";
        break;
    default:
        return -1;
}

    p = kmalloc(sizeof(Passenger) * 1, __GFP_RECLAIM);
    if (p == NULL)
        return -ENOMEM;

    p->id = type;
    p->weight = weight;
    p->name = name;

    // added to back cuz FIFO
    list_add_tail(&p->list, &passengers.list);


    passengers.total_cnt += 1;
    passengers.total_weight += weight;

    return 0;
}

int del_passenger(int type)
{


    return 0;
}

// mutex thing idk
static DEFINE_MUTEX(elevator_mutex);
static int elevator_thread_function(void *data) {
    // This thread will run until it's told to stop
    while (!kthread_should_stop()) {
        // Lock the mutex before accessing/changing shared data
        mutex_lock(&elevator_mutex);

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
        }

        // After processing, release the mutex
        mutex_unlock(&elevator_mutex);

        // Sleep for a bit before the next iteration to not hog the CPU
        msleep(20);
    }
    return 0;
}

// H attempt added stuff end

static ssize_t elevator_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    char buf[10000];
    int len = 0;

    len = sprintf(buf, "Elevator state: \n");
    len += sprintf(buf + len, "Current floor: \n");
    len += sprintf(buf + len, "Current load: \n");
    len += sprintf(buf + len, "Elevator status: \n");

    len += sprintf(buf + len, "\n[ ] Floor 6: \n");
    len += sprintf(buf + len, "[ ] Floor 5: \n");
    len += sprintf(buf + len, "[ ] Floor 4: \n");
    len += sprintf(buf + len, "[ ] Floor 3: \n");
    len += sprintf(buf + len, "[ ] Floor 2: \n");
    len += sprintf(buf + len, "[ ] Floor 1: \n");

    len += sprintf(buf + len, "\nNumber of passengers: %d\n", passengers.total_cnt);
    len += sprintf(buf + len, "Number of passengers waiting: \n");
    len += sprintf(buf + len, "Number of passengers serviced: \n");




    return simple_read_from_buffer(ubuf, count, ppos, buf, len); // better than copy_from_user
}

static const struct proc_ops elevator_fops = {
    .proc_read = elevator_read,
};

static int __init elevator_init(void)
{
    // Initialize the elevator state
    mutex_lock(&elevator_mutex);
    my_elevator.current_floor = 1;
    my_elevator.target_floor = 1;
    my_elevator.weight = 0;
    mutex_unlock(&elevator_mutex);

    // Start the elevator thread
    elevator_thread = kthread_run(elevator_thread_function, NULL, "elevator_thread");
    if (IS_ERR(elevator_thread)) {
        // Handle error
        return PTR_ERR(elevator_thread);
    }
    elevator_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &elevator_fops);
    if (!elevator_entry) {
        return -ENOMEM;
    }
    return 0;
}

static void __exit elevator_exit(void)
{
    kthread_stop(elevator_thread);
    proc_remove(elevator_entry);
}

module_init(elevator_init);
module_exit(elevator_exit);
