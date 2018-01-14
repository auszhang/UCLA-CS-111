// NAME: Christopher Aziz
// EMAIL: caziz@ucla.edu
// ID: 000000000

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

void print_error(char *arg, char *file, int err) {
  fprintf(stderr, "Problem with argument \'--%s\'.\nThe file \'%s\' cannot be opened due to error: %s\n", arg, file, strerror(err));
}

void print_usage() {
  fprintf(stderr, "Usage: lab0 [--input <filename>] [--output <filename>] [--segfault] [--catch]\n");
}

// copy from stdin to stdout
void copy_files() {
  int size = 1;
  char *buff = malloc(sizeof(char) * size);
  while (1) {
    // read from stdin
    ssize_t readin = read(0, buff, size);
    if (readin < 0) {
      fprintf(stderr, "Error reading from stdin:%s\n", strerror(errno));
      exit(3);
    }
    // reached EOF
    else if (readin == 0) {
      break;
    }
    // write to stdout
    else if (write(1, buff, size) < 0) {
      fprintf(stderr, "Error writing to stdout:%s\n", strerror(errno));
      exit(3);
    }
  }
  free(buff);
}

// throw segmentation fault
void force_segfault() {
  char *ptr = NULL;
  *ptr = 0;
}

// redirect 'file' to stdout
void create_file(char *file) {
  // create file descriptor
  int ofd = creat(file, 0666);
  if (ofd >= 0) {
    // redirect to stdout
    if (close(1) < 0 ||
	dup(ofd) < 0 ||
	close(ofd)) {
      fprintf(stderr, "Failure to redirect to stdout:%s\n", strerror(errno));
      exit(3);
    }
  } else {
    print_error("output", file, errno);
    exit(3);
  }
}

// redirect 'file' to stdin
void open_file(char *file) {
  // open file descriptor
  int ifd = open(file, O_RDONLY);
  if (ifd >= 0) {
    // redirect to stdin
    if (close(0) < 0 ||
	dup(ifd) < 0 ||
	close(ifd)) {
      fprintf(stderr, "Failure to redirect to stdin:%s\n", strerror(errno));
      exit(2);
    }
  } else {
    print_error("input", file, errno);
    exit(2);
  }
}

// signal handler
void handle_sig(int sig) {
  // catch segmentation fault
  if (sig == SIGSEGV) {
    fprintf(stderr, "Segementation fault caught by signal handler\n");
    exit(4);
  }
}

// getopt use is modeled after example from gnu.org
// https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html
int main(int argc, char *const argv[]) {
    static struct option long_options[] =
      {
        {"input",   required_argument,  0, 'i'},
        {"output",  required_argument,  0, 'o'},
        {"segfault",no_argument,        0, 's'},
        {"catch",   no_argument,        0, 'c'},
        {0,         0,                  0,  0 }
      };

    // flags
    int iflag = 0;
    int oflag = 0;
    int sflag = 0;
    int cflag = 0;
    
    // file names
    char *ifile = NULL;
    char *ofile = NULL;

    while (1) {
      // current option
      int option = getopt_long(argc, argv, "", long_options, 0);
        
      // reached end of options
      if (option == -1)
	break;
        
      // set flag/file for option
      switch (option) {
      case 'i':
	iflag++;
	ifile = optarg;
	break;
      case 'o':
	oflag++;
	ofile = optarg;
	break;
      case 's':
	sflag++;
	break;
      case 'c':
	cflag++;
	break;
      default:
	// invalid arguments
	printf("Error: unrecognized argument\n");
	print_usage();
	exit(1);
	break;
      }
    }

    // perform options
    if (cflag)
      signal(SIGSEGV, handle_sig);
    if (sflag)
      force_segfault();
    if (iflag)
      open_file(ifile);
    if (oflag)
      create_file(ofile);
    
    copy_files();
    exit(0);
}
