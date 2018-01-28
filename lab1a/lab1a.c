// NAME: Christopher Aziz
// EMAIL: caziz@ucla.edu
// ID: 000000000
//
// lab1a.c
//
// Character-at-a-time, full duplex terminal I/O
// that passes input and output between two processes
// with shutdown processing and error checking.
//

#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

struct termios old_terminal_mode;
int shflag = 0;
pid_t child_pid = 0;
int to_child_pipe[2];
int from_child_pipe[2];

void handle_sig(int sig) {
    if (sig == SIGPIPE) {
        exit(0);
    }
}

void fatal_usage() {
    fprintf(stderr, "Usage: lab1a [--shell]\n");
    exit(1);
}

void fatal_error(char *error_msg) {
    perror(error_msg);
    exit(1);
}

void restore_terminal_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal_mode);
}

void set_terminal_mode() {
    // save old terminal state
    tcgetattr(STDIN_FILENO, &old_terminal_mode);
    atexit(restore_terminal_mode);
    
    // create new terminal state
    struct termios new_terminal_mode = old_terminal_mode;
    new_terminal_mode.c_iflag = ISTRIP;    /* only lower 7 bits    */
    new_terminal_mode.c_oflag = 0;         /* no processing        */
    new_terminal_mode.c_lflag = 0;         /* no processing        */
    
    // set new terminal state
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_terminal_mode) < 0)
        fatal_error("Failed to set parameters associated with the terminal");
}

void read_write() {
    const ssize_t read_buff_size = 256;
    char read_buff[read_buff_size];
    char write_buff[read_buff_size * 2];
    
    while (1) {
        ssize_t readin = read(STDIN_FILENO, read_buff, read_buff_size);
        ssize_t write_buff_size = 0;
        if (readin < 0)
            fatal_error("Error reading from STDIN");
        int i;
        for (i = 0; i < readin; i++) {
            char c = read_buff[i];
            switch (c) {
                case '\r':
                    /* FALLTHRU */
                case '\n':
                    write_buff[write_buff_size++] = '\r';
                    write_buff[write_buff_size++] = '\n';
                    break;
                case 0x04:      // ^D
                    exit(0);
                case 0x03:      // ^C
                    /* FALLTHRU */
                default:
                    write_buff[write_buff_size++] = c;
            }
        }
        if (write(STDOUT_FILENO, write_buff, write_buff_size) < 0)
            fatal_error("Error writing to STDOUT");
    }
}
 
void read_write_shell() {
    struct pollfd pollfds[2];
    // poll keyboard/terminal
    pollfds[0].fd = STDIN_FILENO;
    pollfds[0].events = POLLIN | POLLHUP | POLLERR;
    // poll shell
    pollfds[1].fd = from_child_pipe[0];
    pollfds[1].events = POLLIN | POLLHUP | POLLERR;
    
    // read/write buffers
    const ssize_t read_buff_size = 256;
    char read_buff[read_buff_size];
    // doubled size in case of <lf> and <cr>
    char write_terminal[read_buff_size * 2];
    char write_shell[read_buff_size * 2];

    // flags
    int shutdown_flag = 0;
    int kill_child_flag = 0;    // slightly grim naming
    
    while (1) {
        int poll_ret = poll(pollfds, 2, 0);
        if (poll_ret < 0)   // poll error
            fatal_error("Error polling");
        if (poll_ret == 0)  // nothing polled
            continue;
        
        // check revents and read to read_buff
        ssize_t terminal_readin = 0;
        ssize_t shell_readin = 0;
        if (pollfds[0].revents)
            terminal_readin = read(pollfds[0].fd,
                                   read_buff,
                                   read_buff_size);
        if (terminal_readin < 0)
            fatal_error("Error reading from terminal");
        if (pollfds[1].revents)
            shell_readin = read(pollfds[1].fd,
                                read_buff + terminal_readin,
                                read_buff_size - terminal_readin);
        if (shell_readin < 0)
            fatal_error("Error reading from shell pipe");
        
        // iterate through read characters and build write buffers
        ssize_t write_terminal_size = 0;
        ssize_t write_shell_size = 0;
        int i;
        for (i = 0; i < shell_readin + terminal_readin; i++) {
            // boolean representing if character came from terminal or shell
            int fromTerminal = i < terminal_readin;
            // single character read from buffer
            char c = read_buff[i];
            switch (c) {
                case '\n':
                    if (!fromTerminal) {
                        write_terminal[write_terminal_size++] = '\r';
                        write_terminal[write_terminal_size++] = '\n';
                        break;
                    }
                    /* FALLTHRU */
                case '\r':
                    if (fromTerminal) {
                        write_shell[write_shell_size++] = '\n';
                        write_terminal[write_terminal_size++] = '\r';
                        write_terminal[write_terminal_size++] = '\n';
                    }
                    break;
                case 0x03:      // ^C
                    write_terminal[write_terminal_size++] = '^';
                    write_terminal[write_terminal_size++] = 'C';
                    write_terminal[write_terminal_size++] = child_pid;
                    kill_child_flag = 1;    // this is buggy!
                    break;
                case 0x04:      // ^D
                    write_terminal[write_terminal_size++] = '^';
                    write_terminal[write_terminal_size++] = 'D';
                    shutdown_flag = 1;
                    break;
                default:
                    if (fromTerminal) {
                        write_shell[write_shell_size++] = c;
                        write_terminal[write_terminal_size++] = c;
                    } else {
                        write_terminal[write_terminal_size++] = c;
                    }
            }
        }
        // write when buffer's size is nonzero
        if (write_terminal_size)
            if (write(STDOUT_FILENO, write_terminal, write_terminal_size) < 0)
                fatal_error("Error writing to terminal");
        if (write_shell_size)
            if (write(to_child_pipe[1], write_shell, write_shell_size) < 0)
                fatal_error("Error writing to shell process");
        
        // respond to flags
        if (kill_child_flag)
            if (kill(child_pid, SIGINT) < 0)
                fatal_error("Failed to kill child shell process");
        if (pollfds[1].revents & POLLERR || pollfds[1].revents & POLLHUP || shutdown_flag) {
            if (close(to_child_pipe[1]) < 0)
                fatal_error("Failed to close pipe to shell process on exit");
            int status;
            if (waitpid(child_pid, &status, 0) < 0)
                fatal_error("Error with system call waitpid");
            if (WIFEXITED(status))
                fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status), WEXITSTATUS(status));
            exit(0);
        }
    }
}

// parses options and returns shell flag
void parse_options(int argc, char **argv) {
    struct option longopts[2] =
    {
        {"shell", no_argument, &shflag, 1},
        {      0,           0,       0, 0}
    };
    
    while (1) {
        int option = getopt_long(argc, argv, "", longopts, NULL);
        if (option == -1)       // end of options
            break;
        if (option == 0)        // getopt set flag
            continue;
        if (option == '?') {    // invalid option
            fprintf(stderr, "Error: unknown option\n");
            fatal_usage();
        }
    }
}

void create_shell_process() {
    if (signal(SIGPIPE, handle_sig) == SIG_ERR)
        fatal_error("Failed to create signal handler for SIGPIPE");
    
    if (pipe(to_child_pipe)   < 0 ||
        pipe(from_child_pipe) < 0 )
        fatal_error("Failed to set up pipes between processes");
    
    child_pid = fork();
    
    if (child_pid < 0) {
        fatal_error("Failed to create new process");
    } else if (child_pid == 0) {    // child process
        // close unused pipes
        if  ((close(to_child_pipe[1])   < 0) ||
             (close(from_child_pipe[0]) < 0))
            fatal_error("Failed to close unused pipes in child process");
        
        // redirect standard fileno to pipes
        if ((dup2(to_child_pipe[0],   STDIN_FILENO)  < 0) ||
            (dup2(from_child_pipe[1], STDOUT_FILENO) < 0) ||
            (dup2(from_child_pipe[1], STDERR_FILENO) < 0))
            fatal_error("File redirection failed in child process");
        
        // close original pipe fileno
        if  ((close(to_child_pipe[0])   < 0) ||
             (close(from_child_pipe[1]) < 0))
            fatal_error("Failed to close redirected pipes in child process");
        
        // start shell process
        char *execvp_filename = "/bin/bash";
        char *execvp_argv[] = {execvp_filename, NULL};
        if (execvp(execvp_filename, execvp_argv) < 0)
            fatal_error("Failed replace child process with shell process image");
    } else if (child_pid > 0) { // parent process
        // close unused pipes
        if  ((close(to_child_pipe[0])   < 0) ||
             (close(from_child_pipe[1]) < 0))
            fatal_error("Failed to close unused pipes in parent process");
    }
}

int main(int argc, char **argv) {
    set_terminal_mode();
    parse_options(argc, argv);
    if (shflag) {
        create_shell_process();
        read_write_shell();
    } else {
        read_write();
    }
    return 0;
}
