#include "ticker.h"

#include <poll.h>

#include "debug.h"
#include "helpers.h"
#include "store.h"
#include "watcher.h"

static int cont_running = 1;
volatile int signal_received = 0x0;
// head of the linked list of file descriptors to read
// fd_node* fd_head = NULL;
// signals to watch for
int watched_signals[] = {SIGINT, SIGIO, SIGCHLD};
size_t watched_signals_length =
    sizeof(watched_signals) / sizeof(watched_signals[0]);
struct sigaction sighandler;
sigset_t mask;
sigset_t sec_mask;

void signal_handler(int signum, siginfo_t* siginfo, void* context) {
  switch (signum) {
    // watched_signals[0]
    case SIGINT:
      // debug("SIGINT received");
      signal_received |= HANDLE_SIGINT;
      break;
    // watched_signals[1]
    case SIGIO:
      signal_received |= HANDLE_SIGIO;
      info("SIGIO received on fd %d", siginfo->si_fd);
      break;
    // watched_signals[2]
    case SIGCHLD:
      // lol rip dead children
      error("how many children must die");
      signal_received |= HANDLE_SIGCHILD;
      break;
    // watched_signals[3]
    // case SIGABRT:
    //   // only graceful termination is allowed
    //   error("abortion");
    //   exit(0);
    //   break;
    // unhandled signals
    default:
      debug("Signal %d received", signum);
      break;
  }
}

void append_signals(int signals[]) {
  // debug("watched_signals_length: %ld", watched_signals_length);
  for (int i = 0; i < watched_signals_length; i++) {
    // debug("signal[i]: %d", signals[i]);
    if (sigaction(signals[i], &sighandler, NULL) == -1) {
      debug("sigaction: %d", signals[i]);
      exit(EXIT_FAILURE);
    }
    // mask contains the set of signals to be listened to
    if (sigaddset(&mask, signals[i]) == -1) {
      debug("sigaddset: %d", signals[i]);
      exit(EXIT_FAILURE);
    }
  }
}

void setup_signal_handler(int supported_signals[]) {
  // Set up signal handler for SIGINT
  sighandler.sa_sigaction = signal_handler;
  sigemptyset(&sighandler.sa_mask);
  sighandler.sa_flags = SA_SIGINFO;
  // Wait for SIGACTION and execute do_something()
  sigemptyset(&mask);
  append_signals(supported_signals);
}

int ticker(void) {
  // set up watcher table
  watcher_table = malloc(sizeof(struct WATCHER_TABLE));
  watcher_table->head = NULL;
  watcher_table->length = 0;
  // initialize the watcher types
  watcher_types_init();

  setup_signal_handler(watched_signals);

  // using sigprocmask to block
  sigprocmask(SIG_BLOCK, &mask, &sec_mask);

  // after cli is created, check if stdin has any input from before I started
  // reading if so, read it and process the command

  signal_received |= HANDLE_SIGIO;

  // waits for signals via sigsuspend
  while (cont_running) {
    if (signal_received & HANDLE_SIGIO) {
      signal_received &= ~HANDLE_SIGIO;
      WATCHER* next = watcher_table->head;
      while (next != NULL) {
        // debug("next->read: %d", next->read);
        struct pollfd fd = {.fd = next->read, .events = POLLIN};
        int ret = poll(&fd, 1, 0);
        if (ret == -1) {
          // Handle the error here
          debug("poll error");
        } else if (ret != 0) {
          // info("sigio is being cleared on fd %d", next->read);
          // The file descriptor is ready for reading
          // Proceed with reading from the file descriptor

          // init memstream
          char* memstream_buf;
          size_t memstream_buf_size;
          FILE* memstream_fd =
              open_memstream(&memstream_buf, &memstream_buf_size);
          if (memstream_fd == NULL) {
            // Handle the error here, for example by logging an error message
            // and exiting the program
            debug("Failed to allocate memory for memstream\n");
            exit(EXIT_FAILURE);
          }
          char buf[1024];
          memset(buf, 0, sizeof(buf));
          ssize_t nbytes = -2;
          errno = 0;
          while (nbytes != -1 && nbytes != 0) {
            nbytes = read(next->read, buf, sizeof(buf) / sizeof(char));
            if (nbytes != -1) {
              // fprintf(memstream_fd, "%s", buf);
              fwrite(buf, sizeof(char), nbytes, memstream_fd);
            }
            // make sure that read doesnt contain EOF causing infinite loop
            if (nbytes == 0) {
              if (strcmp(next->type->name, "CLI") == 0) {
                debug("EOF received");
                signal_received |= HANDLE_SIGINT;
              }
            }
          }
          errno = 0;
          // debug("MEMSTREAM SIZE: %ld", memstream_buf_size);
          // flush the memstream
          fflush(memstream_fd);
          fclose(memstream_fd);
          // memstream not taking inputs

          int buf_len = 0;
          while (memstream_buf[buf_len] != '\0') {
            buf_len++;
          }
          memstream_buf = realloc(memstream_buf, (buf_len + 1) * sizeof(char));

          size_t prev_buf_size = 0;
          if (next->buffer != NULL) {
            prev_buf_size = strlen(next->buffer);
          }

          buf_len += prev_buf_size;
          char* cmd_buffer = malloc((buf_len + 1) * sizeof(char));
          memset(cmd_buffer, 0, (buf_len + 1) * sizeof(char));
          if (next->buffer != NULL) {
            sprintf(cmd_buffer, "%s%s", next->buffer, memstream_buf);
            free(next->buffer);
            next->buffer = NULL;
          } else {
            sprintf(cmd_buffer, "%s", memstream_buf);
          }
          free(memstream_buf);
          // info("cmd_buffer: %s", cmd_buffer);

          int initial_size = 100;
          char** array_of_commands = malloc(sizeof(char*) * initial_size);
          int array_of_commands_length = 1;
          int incmplt_cmd = 0;
          if (cmd_buffer[buf_len - 1] != '\n') {
            // debug("incomplete command");
            incmplt_cmd = 1;
          } else {
            warn("buf_len: %d", buf_len);
            if (strcmp(cmd_buffer, "\0") != 0) cmd_buffer[buf_len - 1] = '\0';
          }
          int i = 0;
          array_of_commands[0] = &cmd_buffer[0];
          while (cmd_buffer[i] != '\0') {
            if (cmd_buffer[i] == '\n' && i < buf_len - 1) {
              cmd_buffer[i] = '\0';
              array_of_commands[array_of_commands_length] = &cmd_buffer[i + 1];
              array_of_commands_length++;
              if (array_of_commands_length >= initial_size) {
                initial_size *= 2;
                array_of_commands =
                    realloc(array_of_commands, sizeof(char*) * initial_size);
              }
            }
            i++;
          }
          array_of_commands_length++;
          array_of_commands[array_of_commands_length-1] = NULL;
          i = 0;
          // debug("%s", array_of_commands[0]);
          while (array_of_commands[i] != NULL) {
            if (strcmp(array_of_commands[i], "\0") == 0) {
              i++;
              if (array_of_commands_length <= 2 && strcmp(next->type->name, "CLI") == 0) next->type->send(next, "???\n"); 
              continue;
            }
            if (i+1 == array_of_commands_length - 1 && incmplt_cmd) {
              next->buffer = strdup(array_of_commands[i]);
              // debug("buffer: %s", next->buffer);
            } else {
              next->type->recv(next, array_of_commands[i]);
            }
            i++;
          }

          free(array_of_commands);
          free(cmd_buffer);
        }
        next = next->next;
      }
    }

    // g-d has signaled the binding of isaac
    if (signal_received & HANDLE_SIGCHILD) {
      // close file descriptors and remove from watcher table
      signal_received &= ~HANDLE_SIGCHILD;
      wait_for_dead_child();
    }

    if (signal_received & HANDLE_SIGINT) {
      // handle SIGINT includes calls to terminate gracefully
      signal_received &= ~HANDLE_SIGINT;
      cont_running = 0;
    }

    if (cont_running) {
      sigsuspend(&sec_mask);
    }
  }

  sigprocmask(SIG_SETMASK, &sec_mask, NULL);  // unblock

  // Cleanup program resources here
  // loop through watcher list and shut each down
  destroy_all_processes();
  return 0;
}
