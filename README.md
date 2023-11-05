# Dorm Elevator

This project aims to provide a comprehensive understanding of system calls, kernel programming, concurrency, synchronization, and elevator scheduling algorithms. 

## Group Members
- **Margaret Rivas**: mer20c@fsu.edu
- **Sophia Quinoa**: saq20a@fsu.edu
- **Hannah Housand**: hjh21a@fsu.edu

## Division of Labor

### Part 1: System Call Tracing
- **Responsibilities**:
- [X] Create a baseline C/Rust program named "empty" with no system calls.
- [X] Duplicate the "empty" program, naming the new copy "part1", and add exactly four system calls.
- [X] Use the strace tool to ensure the correct number of system calls are added to "part1".
- Completed by:
- **Assigned to**: Hannah Housand, sophia quinoa


### Part 2: Timer Kernel Module
- **Responsibilities**:
- [X] Develop a kernel module that uses ktime_get_real_ts64() to retrieve the current time in seconds and nanoseconds since the Unix Epoch.
- [X] Ensure the module creates a proc entry "/proc/timer" when loaded and removes it when unloaded.
- [X] Implement a read operation for "/proc/timer" to print the current time and the elapsed time since the last read.
- Completed by:
- **Assigned to**: Hannah Housand, sophia quinoa

### Part 3a: Adding System Calls
- **Responsibilities**:
- [X] Prepare kernel for compilation.
- [X] Modify kernel files.
- [X] Define system calls.
- Completed by:
- **Assigned to**: Hannah Housand, sophia quinoa

### Part 3b: Kernel Compilation
- **Responsibilities**:
- [X] Compile kernel with new system calls, disabling certificates.
- [X] Check if installed.
- Completed by: Hannah Housand, Margaret Rivas
- **Assigned to**: Hannah Housand, Margaret Rivas


### Part 3c: Threads
- **Responsibilities**:
- [ ] Use a kthread to control the elevator movement.
- Completed by:
- **Assigned to**: Hannah Housand, sophia quinoa

### Part 3d: Linked List
- **Responsibilities**:
- [ ] Use linked lists to handle the number of passengers per floor/elevator.
- Completed by:
- **Assigned to**: Hannah Housand, Margaret Rivas

### Part 3e: Mutexes
- **Responsibilities**:
- [ ] Use a mutex to control shared data access between floor and elevators.
- Completed by:
- **Assigned to**: sophia quinoa, Margaret Rivas

### Part 3f: Scheduling Algorithm
- **Responsibilities**:
- [ ] Develop algorithm.
- [ ] Use kmalloc to allocate dynamic memory for passengers.
- Completed by:
- **Assigned to**: sophia quinoa, Margaret Rivas

## File Listing
```
elevator/
├── Makefile
├── part1/
│   ├── empty.c
│   ├── empty.trace
│   ├── part1.c
│   ├── part1.trace
│   └── Makefile
├── part2/
│   ├── src/
│   │   ├── my_timer.c
│   │   └── Makefile
│   │   ├── my_timer.h
│   └── Makefile
├── part3/
│   ├── src/
│   │   ├── elevator.c
│   │   └── Makefile
│   ├── tests/
│   ├── Makefile
│   └── syscalls.c
├── Makefile
└── README.md

```
# How to Compile & Execute

### Requirements
- **Compiler**: e.g., `gcc` for C/C++, `rustc` for Rust.
- **Dependencies**: List any libraries or frameworks necessary (rust only).

## Part 1

### Compilation
```bash
make
```
This will build the executable in part1/
### Execution
```bash
make run
```
This will run the program ...

## Part 2

### Compilation
For a C/C++ example:
```bash
make
```
This will build the executable in part2/src/
### Execution
```bash
make run
```
This will run the program ...


## Part 3

### Compilation
For a C/C++ example:
```bash
make
```
This will build the executable in part3/src/
### Execution
```bash
make run
```
This will run the program ...


## Bugs
- **Bug 1**: This is bug 1.
- **Bug 2**: This is bug 2.
- **Bug 3**: This is bug 3.

## Considerations
- The proc file will only be 10000 characters long.
- Students will not leave from the same floor they entered from.
