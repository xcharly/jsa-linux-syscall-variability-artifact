/* randbyte.c: program under test.
 *
 * Based on the program randbyte of I. Allende's PhD thesis (Appendix A.1), and
 * modified by Markel Galarraga.
 *
 * The program enables ftrace by itself: it sets set_ftrace_pid to its own PID and
 * writes 1 to tracing_on (the DB4SIL2 client does not start tracing). It then opens
 * /dev/urandom and /dev/null, reads one byte from the former, writes it to the
 * latter and closes both files.
 */

#include <fcntl.h> // open
#include <unistd.h> // read, write, close, getpid
#include <stdio.h> // sprintf
#include <stdlib.h> // system

int main()
{
    int fd_urandom, fd_null;
    unsigned char c;

    char command[50];
    pid_t process_id;

    // start tracing: only this process, from here on
    process_id = getpid();
    sprintf(command, "echo %d > /sys/kernel/tracing/set_ftrace_pid", process_id);
    system(command);
    sprintf(command, "echo 1 > /sys/kernel/tracing/tracing_on");
    system(command);

    // open
    fd_urandom = open("/dev/urandom", O_RDONLY);
    fd_null = open("/dev/null", O_WRONLY);

    // read
    read(fd_urandom, &c, 1);

    // write
    write(fd_null, &c, 1);

    // close
    close(fd_urandom);
    close(fd_null);
}
