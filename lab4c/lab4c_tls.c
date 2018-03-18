//  NAME: Christopher Aziz
//  EMAIL: caziz@ucla.edu
//
//  lab4c_tls.c
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
#include <netinet/in.h>
#include <netdb.h>

#include <mraa.h>
#include <mraa/aio.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#define SLEEP 0
#define EXIT_SUCCESS 0
#define EXIT_ARGS 1
#define EXIT_FAIL 2

char scale = 'F';
int log_opt = 0;
int port_num;
int stopped = 0;
char id [64];
char host_name [64];
int kill_process = 0;
int socketfd;
long period = 1;    // in seconds
FILE *logfp = 0;
mraa_aio_context temp_context;
SSL_CTX *ssl_ctx;
SSL *ssl;

// invalid command-line parameter/argument
void fatal_usage() {
    fprintf(stderr, "Usage: lab4c_tls portnum --id=number --host=name/address [--period=value] [--scale=[F,C]]] [--log=filename]\n");
    exit(EXIT_ARGS);
}

// system call error or other failure
void fatal_error(char *error_msg) {
    perror(error_msg);
    exit(EXIT_FAIL);
}

// todo: fix exit codes
void connect_to_server() {
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0)
        fatal_error("Failed to open socket");
    struct hostent *server = gethostbyname(host_name);
    if (server == NULL)
        fatal_error("Failed to get host by name");
    struct sockaddr_in server_address;
    memset((char *) &server_address, 0, sizeof(server_address));
    
    // set server_address fields
    server_address.sin_family = AF_INET;
    // copy server address string to server_address
    memmove(&server_address.sin_addr.s_addr,
            server->h_addr,
            server->h_length);
    // set port number to port number converted to network byte order
    server_address.sin_port = htons(port_num);
    if (connect(socketfd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
        fatal_error("Error connecting client to server");
}

void parse_options(int argc, char **argv) {
    struct option longopts[6] =
    {
        {"period",     required_argument, 0, 'p'},  // optional
        {"scale",      required_argument, 0, 's'},  // optional
        {"log",        required_argument, 0, 'l'},  // optional
        {"id",         required_argument, 0, 'i'},  // required
        {"host",       required_argument, 0, 'h'},  // required
        {0,             0,                0,  0 }
    };
    int hflag = 0;
    int iflag = 0;
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
                if (logfp == NULL) {
                    perror("Failed to open log file");
                    exit(EXIT_ARGS);
                }
                break;
            case 'i':
                iflag++;
                sprintf(id, "ID=%s\n", optarg);
                break;
            case 'h':
                hflag++;
                strcpy(host_name, optarg);
                break;
            case '?':                   // invalid option
                /* FALLTHRU */
            default:
                fprintf(stderr, "Invalid option\n");
                fatal_usage();
        }
    }
    if (!hflag || !iflag || optind == argc) {
        fprintf(stderr, "Missing required parameter\n");
        fatal_usage();
    }
    // save non-switch parameter
    port_num = atoi(argv[optind]);
}

void write_log(char *message) {
    if (log_opt) {
        fputs(message, logfp);
        fflush(logfp);
    }
}

void write_server(char *message) {
    int ret = SSL_write(ssl, message, strlen(message));
    if (ret < 0)
        fatal_error("Failed to write over SSL connection");
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
        kill_process = 1;
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
                fprintf(stderr, "Invalid command!\n");
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

void handle_shutdown() {
    char sdbuf[128];
    struct timeval time;
    gettimeofday(&time, 0);
    struct tm *now = localtime(&(time.tv_sec));
    sprintf(sdbuf, "%02d:%02d:%02d SHUTDOWN",
            now->tm_hour,
            now->tm_min,
            now->tm_sec);
    write_server(sdbuf);
    if(log_opt)
        write_log(sdbuf);
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

void init_ssl() {
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    if (SSL_library_init() < 0)
        fatal_error("Failure initializing OpenSSL library");
    const SSL_METHOD *ssl_method = SSLv23_client_method();
    ssl_ctx = SSL_CTX_new(ssl_method);
    if ((ssl_ctx) == NULL)
        fatal_error("Failure creating an SSL context structure");
    ssl = SSL_new(ssl_ctx);
    
    SSL_set_fd(ssl, socketfd);
    if (SSL_connect(ssl) != 1)
        fatal_error("Failure building an SSL session");
}

int main(int argc, char **argv) {
    
    parse_options(argc, argv);
    connect_to_server();
    init_ssl();
    
    /* initialize I/O pins*/
    temp_context = mraa_aio_init(1);
    struct pollfd poll_server;
    poll_server.fd = socketfd;
    poll_server.events = POLLIN | POLLHUP | POLLERR;
    
    char cmdbuf[128];
    char outbuf[128];
    struct timeval timer;
    struct timeval now;
    gettimeofday(&timer, 0);
    // generate first report before possibly executing commands
    int generated_first_report = 0;
    
    // immediately send (and log) ID
    write_server(id);
    write_log(id);
    while(1) {
        if (kill_process) {
            handle_shutdown();
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
            write_server(outbuf);
            if(!stopped) {
                write_log(outbuf);
            }
            gettimeofday(&timer, 0);
            generated_first_report = 1;
        }
        // listen for commands
        poll_server.revents = 0;
        int pollin = poll(&poll_server, 1, SLEEP);
        if (pollin < 0)
            fatal_error("Error while polling server");
        if (pollin >= 1 && generated_first_report) {
            int readin = SSL_read(ssl, cmdbuf, sizeof(cmdbuf));
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
                if(kill_process) {
                    handle_shutdown();
                    break;
                }
            } else { // EOF
                break;
            }
        }
    }
    if(log_opt)
        fclose(logfp);
    exit(EXIT_SUCCESS);
}

