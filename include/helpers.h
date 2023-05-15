#include "debug.h"
#include "watcher.h"
#include "store.h"
#include <strings.h>
#include <signal.h>
#include <string.h>

#ifndef HELPERS_H
#define HELPERS_H

// GLOBALS
extern int* available_ids;
extern int available_ids_size;
// FUNCTIONS
extern void init_available_ids();
extern int double_available_ids();
extern int get_available_id();
extern int id_is_assigned(int id);
extern int free_id(int id);
extern int watcherTypeIsNot0(WATCHER_TYPE *a);
extern int powerOfTwo(int n);
extern int fromPowerOfTwo(int n);
extern int ends_with_volume(const char *str);
extern int ends_with_price(const char *str);
//SIGNALS
extern void signal_handler(int signum, siginfo_t *siginfo, void *context);
extern void append_signals(int signals[]);
extern void setup_signal_handler(int supported_signals[]);
// STORE
// extern void add_data(char* key, char* value);
// extern char* get_data(char* key);
extern void remove_data(char* key);

// extern void add_int_data(char* key, long value);
// extern long get_int_data(char* key);
extern void add_double_data(char* key, double value);
extern double get_double_data(char* key);

#endif