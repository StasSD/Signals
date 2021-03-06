#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>


//-------------------- for debugging
#define BYTE_SIZE 8
#define INT_SIZE BYTE_SIZE * 4

#define DEBUG 0
#define SLP 0

#define DBG(expr) if (DEBUG) {expr;}
#define SLEEP if (SLP) {sleep(1);}


unsigned count = 0;
unsigned is_read = 0;
unsigned input_size = 0;
char* buf = NULL;
char current_byte = 0;
unsigned current_len = 0;
int fd = 0;

void handler(int bit) {

    DBG(fprintf(stderr, "0\n");)
    if (is_read == 0) {
        input_size = input_size << 1;
        if (bit == 1) {
            input_size++;
        }
    }

    if (is_read == 1) {
        current_byte = current_byte << 1;
        if (bit == 1) {
            current_byte++;
        }
    }

    if (count == INT_SIZE - 1 && is_read == 0) {
        DBG(fprintf(stderr, "\nSize received: %d\n", input_size))
        buf = (char*) calloc(input_size, sizeof(char));
        assert(buf != NULL);
        is_read  = 1;
        count = 0;
        return;
    }

    if (count == BYTE_SIZE - 1 && is_read == 1) {
        DBG(fprintf(stderr, "\nByte received: %c\n", current_byte))
        buf[current_len] = current_byte;
        current_len++;
        count = 0;
        return;
    }

    if (count < INT_SIZE && is_read == 0) {
         count++;
    }

    if (count < BYTE_SIZE && is_read == 1) {
         count++;
    }

}


int main (int argc, char** argv) {

    assert(argc == 2);
    char* file = argv[1];

    fd = open(file, O_WRONLY | O_CREAT, 0666);
    assert(fd > 0);

    int pid = getpid();
    fprintf(stdout, "PID: %d\n", pid);

    int res = -1;

//-------------------- setting signals for appropriate handling
    sigset_t waitset;
    siginfo_t siginfo;

    sigemptyset(&waitset);
    sigaddset(&waitset, SIGUSR1);
    sigaddset(&waitset, SIGUSR2);
    sigaddset(&waitset, SIGTERM);

    res = sigprocmask(SIG_BLOCK, &waitset, NULL);
    if (res == -1) {
        DBG(fprintf(stderr, "Cannot block signals\n"))
        exit(-1);
    }

    while (1) {
        res = sigwaitinfo(&waitset, &siginfo); // waiting for transmitter
        assert(res != -1);

        switch(res) {
            case SIGUSR1:
                DBG(fprintf(stderr, "SIGUSR1 received!\n")) // manually passing control to handlers
                handler(0);
            break;
            case SIGUSR2:
                DBG(fprintf(stderr, "SIGUSR2 received!\n"))
                handler(1);
            break;
            case SIGTERM:
                buf[input_size] = '\0';
                fprintf(stdout,"Input_size: %d\n", input_size);
                fprintf(stdout,"End of transmission.\n");
                int n_write = write(fd, buf, input_size * sizeof(char)); 
                assert(n_write == input_size);
                free(buf);
                close(fd);
                exit(0);
            break;
        }

        SLEEP
        DBG(fprintf(stderr, "Sending response to %d\n", siginfo.si_pid))
        kill(siginfo.si_pid, SIGUSR1);
    }

    return 0;
}