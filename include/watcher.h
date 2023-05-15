#include "ticker.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#ifndef WATCHER_H
#define WATCHER_H

struct WATCHER_TABLE {
  int length;
  WATCHER* head;
};
/*
 * Singly linked list of watchers
 */
extern struct WATCHER_TABLE *watcher_table;
/*
 receives signals to handle in the main loop
 */
extern volatile int signal_received;
// tells the main loop there is data to read
#define HANDLE_SIGIO 0x1
// tells the main loop to exit gracefully
#define HANDLE_SIGINT 0x2
// tells the main loop to reap children
#define HANDLE_SIGCHILD 0x4


typedef struct watcher {
    // display id of the water
    int id;
    int serial;
    pid_t pid;
    int trace;
    char** args;
    char* buffer;
    char* volume_path;
    char* price_path;
    // int pipe[2];
    WATCHER_TYPE *type;
    WATCHER* next;
    /* writing to out */
    int write;
    /* reading from in */
    int read;
} WATCHER;
extern WATCHER_TYPE* find_watcher_type(char *type);
extern void watcher_types_init();
extern void add_watcher(WATCHER* watcher);
extern void remove_watcher(WATCHER* watcher);
extern void print_watcher_table(WATCHER* watcher);
extern void destroy_all_processes();
extern WATCHER* get_watcher(int id);
extern WATCHER* get_watcher_by_pid(pid_t pid);
extern int wait_for_dead_child();
extern void free_wp_args(char** args);
extern void trace_program(WATCHER* wp, char* txt);

// extern void execute_command(WATCHER* cli_watcher, char* command_buffer);
// extern void create_watcher(WATCHER_TYPE* type, char* channel);
extern void destroy_watcher(int id);
extern void dispose_of_parent(WATCHER* wp);

#include "cli.h"
#include "bitstamp.h"
#endif