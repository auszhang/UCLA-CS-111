//  NAME: Christopher Aziz
//  EMAIL: caziz@ucla.edu
//
//  lab4b.c
//

#include <stdio.h>
#include <getopt.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>

#include <mraa.h>
#include <mraa/aio.h>

#define SLEEP 0
#define EXIT_OK 0
#define EXIT_ARGS 1
#define EXIT_FAIL 1
                     
char scale = 'F';
int log_opt = 0;
int stopped = 0;
int kill = 0;

long period = 1;    // in seconds
FILE *logfp = 0;
mraa_gpio_context button_context;
mraa_aio_context temp_context;

// invalid command-line parameter/argument
void fatal_usage() {
    fprintf(stderr, "Usage: lab4b [--period=value] [--scale=[F,C]]] [--log=logfile]\n");
    exit(EXIT_ARGS);
}

// system call error or other failure
void fatal_error(char *error_msg, int error_code) {
    perror(error_msg);
    exit(error_code);
}

void parse_options(int argc, char **argv) {
    
    struct option longopts[5] =
    {
        {"period",     required_argument, 0, 'p'},
        {"scale",      required_argument, 0, 's'},
        {"log",        required_argument, 0, 'l'},
        {0,             0,                0,  0 }
    };
    while (1) {
        int option = getopt_long(argc, argv, "p:s:l:", longopts, NULL);
        if (option == -1)               // end of options
            break;
        switch (option) {
            case 0:                     // getopt already set flag
                break;
            case 'p':
                period = atoi(optarg);
                break;
            case 's':
                if (strlen(optarg) != 1) {
                    fprintf(stderr, "Incorrect scale option: %s\n", optarg);
                    fatal_usage();
                }
                switch (optarg[0]) {
                    case 'C':
                    case 'F':
                        scale = optarg[0];
                        break;
                    default:
                        fprintf(stderr, "Incorrect scale option: %s\n", optarg);
                        fatal_usage();
                }
                break;
            case 'l':
                log_opt++;
                logfp = fopen(optarg, "w+");
                if (logfp == NULL)
                    fatal_error("Failed to open log file", EXIT_ARGS);
                break;
            case '?':                   // invalid option
                /* FALLTHRU */
            default:
                fprintf(stderr, "Invalid option\n");
                fatal_usage();
        }
    }
}

void execute_command(char *command) {
    if (strcmp(command, "SCALE=F") == 0) {
        scale = 'F';
    } else if (strcmp(command, "SCALE=C") == 0) {
        scale = 'C';
    } else if (strcmp(command, "STOP") == 0) {
        stopped = 1;
    } else if (strcmp(command, "START") == 0) {
        stopped = 0;
    } else if (strcmp(command, "OFF") == 0) {
        kill = 1;
    } else {
        const char *period_string = "PERIOD=";
        const char *log_string = "LOG ";
        
        char terminated_command [64];
        strcpy(terminated_command, command);
        
        terminated_command[strlen(period_string)] = '\0';
        if (strcmp(terminated_command, period_string) == 0) {
            period = atoi(&command[strlen(period_string)]);
        } else {
            terminated_command[strlen(log_string)] = '\0';
            if (strcmp(terminated_command, log_string) == 0) {
                // just log command
            } else {
                fputs("Invalid command!", stdout);
                return;
            }
        }
    }
    if(log_opt) {
        fputs(command, logfp);
        fputs("\n", logfp);
        fflush(logfp);
    }
}

int read_temp(mraa_aio_context temp_sensor) {
    const int B = 4275;               // B value of the thermistor
    const int R0 = 100000;            // R0 = 100k
    int reading = mraa_aio_read(temp_sensor);
    float R = 1023.0 / reading - 1.0;
    R = R0 * R;
    float temperature = 1.0 / (log(R / R0) / B + 1 / 298.15) - 273.15; // convert to temperature via datasheet
    return scale == 'C' ? temperature : (temperature * 9 / 5) + 32;
}

int main(int argc, char **argv) {
    parse_options(argc, argv);
    
    /* initialize I/O pins */
    temp_context = mraa_aio_init(1);
    button_context = mraa_gpio_init(62);
    mraa_gpio_dir(button_context, MRAA_GPIO_IN);
    struct pollfd poll_stdin = {0, POLLIN, 0};
    char cmdbuf[128];
    char outbuf[128];
    struct timeval timer;
    struct timeval now;
    gettimeofday(&timer, 0);
    // generate first report before possibly executing commands
    int generated_first_report = 0;
    while(1) {
        if(kill || mraa_gpio_read(button_context)) {
            fputs("SHUTDOWN\n", stdout);
            break;
        }
        gettimeofday(&now, 0);
        if (now.tv_sec - timer.tv_sec >= period) {
            float temp = read_temp(temp_context);
            int t = temp * 10;
            
            struct tm *now = localtime(&(timer.tv_sec));
            sprintf(outbuf, "%02d:%02d:%02d %d.%1d\n",
                    now->tm_hour,
                    now->tm_min,
                    now->tm_sec,
                    t / 10,
                    t % 10);
            
            // echo to screen
            fputs(outbuf, stdout);
            fflush(stdout);
            // write to log
            if(log_opt && !stopped) {
                fputs(outbuf, logfp);
                fflush(logfp);
            }
            gettimeofday(&timer, 0);
            generated_first_report = 1;
        }
        // listen for commands
        poll_stdin.revents = 0;
        int pollin = poll(&poll_stdin, 1, SLEEP);
        if(pollin >= 1 && generated_first_report) {
            ssize_t readin = read(0, cmdbuf, sizeof(cmdbuf));
            if (readin > 0) {
                char *s = cmdbuf;
                // parse the command
                while(s < &cmdbuf[readin]) {
                    char *e = s;
                    while(e < &cmdbuf[readin] && *e != '\n')
                        e++;
                    *e = 0;
                    execute_command(s);
                    s = &e[1];
                }
                if(kill) {
                    fputs("SHUTDOWN\n", stdout);
                    break;
                }
            } else {
                break;
            }
        }
    }
    if(log_opt) {
        fputs("SHUTDOWN\n", logfp);
        fclose(logfp);
    }
    exit(EXIT_OK);
}
