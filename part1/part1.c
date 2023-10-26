#include <unistd.h> 

int main() {
    // Four system calls
    getpid();  // Get process ID
    getuid();  // Get user ID
    getgid();  // Get group ID
    getppid(); // Get parent process ID
    
    return 0;
}
