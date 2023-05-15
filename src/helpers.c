#include "helpers.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "cli.h"
#include "debug.h"
#include "store.h"
#include "ticker.h"
#include "watcher.h"

int* available_ids;
int available_ids_size = 100;
struct WATCHER_TABLE* watcher_table;

/*
 * init available_ids array
 */
void init_available_ids() {
  available_ids = malloc(available_ids_size * sizeof(int));
  int i = 0;
  while (i < available_ids_size) {
    available_ids[i] = i + 1;
    i++;
  }
}
/*
 * double the size of available_ids
 * return 1 on success
 * return 0 on failure
 */
int double_available_ids() {
  // else, realloc available_ids to double the size
  if (realloc(available_ids, available_ids_size * sizeof(int) * 2) == 0) {
    return 0;
  }
  // init new ids
  for (int i = available_ids_size; i < available_ids_size * 2; i++) {
    available_ids[i] = i + 1;
  }
  available_ids_size = available_ids_size * 2;
  return 1;
}

/*
 * get smallest available id
 * return id on success
 * return 0 on failure
 */
int get_available_id() {
  int i = 0;
  while (i < available_ids_size) {
    if (available_ids[i] != 0) {
      int id = available_ids[i];
      available_ids[i] = 0;
      return id;
    }
    i++;
  }
  double_available_ids();
  return get_available_id();
}
/*
 * check if an ID has been assigned
 */
int id_is_assigned(int id) {
  if (id > available_ids_size) {
    return 1;
  }
  if (id == 0) {
    return 1;
  }
  if (available_ids[id - 1] == 0) {
    return 1;
  }
  return 0;
}

/*
 * free an id no longer in use
 * return 1 on success
 * return 0 on failure
 */
int free_id(int id) {
  if (id > available_ids_size) {
    return 0;
  }
  if (id < 1) {
    return 0;
  }
  if (available_ids[id - 1] == 0) {
    available_ids[id - 1] = id;
    return 1;
  }
  return 0;
}

/*
 * free id array
 */
void free_available_ids() { free(available_ids); }

int watcherTypeIsNot0(WATCHER_TYPE* a) {
  if (a->name == 0) {
    return 0;
  }
  return 1;
}
/*
 * return the nth power of two
 */
int powerOfTwo(int n) {
  int i = 0;
  while (i < n) {
    i++;
  }
  return 1 << i;
}
/*
 * return the log base 2 of n
 */
int fromPowerOfTwo(int n) {
  int i = 0;
  while (n > 1) {
    n = n >> 1;
    i++;
  }
  return i;
}
/*
 * return the watcher type with the given name
 */
WATCHER_TYPE* find_watcher_type(char* type) {
  int i = 0;
  while (watcherTypeIsNot0(&watcher_types[i])) {
    if (strcmp(watcher_types[i].name, type) == 0) {
      return &watcher_types[i];
    }

    i++;
  }
  return NULL;
}
void watcher_types_init() {
  init_available_ids();
  // set up cli watcher type
  WATCHER* CLI = cli_watcher_start(find_watcher_type("CLI"), NULL);
  add_watcher(CLI);
  return;
}
/*
 * Add watcher to watcher_table singly linked list
 */
void add_watcher(WATCHER* watcher) {
  if (watcher_table->head == NULL) {
    watcher_table->head = watcher;
    watcher_table->length++;
    return;
  }
  WATCHER* current_watcher = watcher_table->head;
  while (current_watcher->next != NULL) {
    if (current_watcher->next->id > watcher->id) {
      watcher->next = current_watcher->next;
      current_watcher->next = watcher;
      watcher_table->length++;
      return;
    }
    current_watcher = current_watcher->next;
  }
  current_watcher->next = watcher;
  watcher_table->length++;
  return;
}

/*
 * Remove watcher from watcher_table singly linked list
 */
void remove_watcher(WATCHER* watcher) {
  if (watcher_table->head == NULL) {
    return;
  }
  if (watcher_table->head == watcher) {
    watcher_table->head = watcher->next;
    watcher_table->length--;
    return;
  }
  WATCHER* current_watcher = watcher_table->head;
  while (current_watcher->next != NULL) {
    if (current_watcher->next == watcher) {
      current_watcher->next = watcher->next;
      watcher_table->length--;
      return;
    }
    current_watcher = current_watcher->next;
  }
  return;
}
/*
 * returns pointer to a watcher in the table with the given type and args
 */
WATCHER* get_watcher(int id) {
  WATCHER* current_watcher = watcher_table->head;
  while (current_watcher != NULL) {
    if (current_watcher->id == id) {
      return current_watcher;
    }
    current_watcher = current_watcher->next;
  }
  return NULL;
}

WATCHER* get_watcher_by_pid(pid_t pid) {
  WATCHER* current_watcher = watcher_table->head;
  while (current_watcher != NULL) {
    if (current_watcher->pid == pid) {
      return current_watcher;
    }
    current_watcher = current_watcher->next;
  }
  return NULL;
}

/*
 * add data to the data store
 */
// void add_data(char* key, char* value) {
//   struct store_value* old_store_value = store_get(key);
//   // if key does not exist, create new store_value
//   if (old_store_value == NULL) {
//     struct store_value* new_store_value = malloc(sizeof(struct store_value));
//     new_store_value->content.string_value =
//         malloc((strlen(value)) * sizeof(char));
//     strcpy(new_store_value->content.string_value, value);
//     new_store_value->type = STORE_STRING_TYPE;
//     store_put(key, new_store_value);
//   }
//   // else append
//   else {
//     char* old_value = old_store_value->content.string_value;
//     old_store_value->content.string_value =
//         realloc(old_store_value->content.string_value,
//                 (strlen(old_value) + strlen(value) + 1) * sizeof(char));
//     strcat(old_store_value->content.string_value, value);
//     store_put(key, old_store_value);
//   }
// }

/*
 * add int data to the data store
 */
// void add_int_data(char* key, long value) {
//   struct store_value* old_store_value = store_get(key);
//   // if key does not exist, create new store_value
//   if (old_store_value == NULL) {
//     struct store_value* new_store_value = malloc(sizeof(struct store_value));
//     new_store_value->content.long_value = value;
//     new_store_value->type = STORE_LONG_TYPE;
//     store_put(key, new_store_value);
//   }
//   // else update
//   else {
//     if (old_store_value->type != STORE_LONG_TYPE) {
//       if (old_store_value->type == STORE_STRING_TYPE) {
//         if (old_store_value->content.string_value != NULL)
//           free(old_store_value->content.string_value);
//         old_store_value->content.string_value = NULL;
//       }
//       old_store_value->type = STORE_DOUBLE_TYPE;
//     }
//     old_store_value->content.long_value = value;
//     store_put(key, old_store_value);
//   }
// }

/*
 * get int data from the data store
 */
// long get_int_data(char* key) {
//   // check if key exists
//   struct store_value* long_store_value = store_get(key);
//   debug("long_store_value: %p", long_store_value);
//   if (long_store_value == NULL) {
//     return -1;
//   } else if (long_store_value->type != STORE_LONG_TYPE) {
//     error("wrong type for long_store_value");
//     return -1.0;
//   }
//   // retrieve value from store
//   long value = long_store_value->content.long_value;
//   return value;
// }

/*
 * add int data to the data store
 */
void add_double_data(char* key, double value) {
  remove_data(key);
  // if key does not exist, create new store_value
  struct store_value* new_store_value = malloc(sizeof(struct store_value));
  new_store_value->type = STORE_DOUBLE_TYPE;
  new_store_value->content.double_value = value;
  store_put(key, new_store_value);
  free(new_store_value);
}

/*
 * get int data from the data store
 */
double get_double_data(char* key) {
  // check if key exists
  struct store_value* double_store_value = store_get(key);
  if (double_store_value == NULL) {
    error("double_store_value is NULL");
    return -1.0;
  } else if (double_store_value->type != STORE_DOUBLE_TYPE) {
    error("wrong type for double_store_value");
    return -1.0;
  }
  // retrieve value from store
  double value = double_store_value->content.double_value;
  free(double_store_value);
  return value;
}

/*
 * get data from the data store
 */
// char* get_data(char* key) {
//   // check if key exists
//   if (store_get(key) == NULL) {
//     return NULL;
//   }
//   // retrieve value from store
//   char* value = store_get(key)->content.string_value;
//   return value;
// }
/*
 * remove data from the data store
 */
void remove_data(char* key) {
  struct store_value* store_value = store_get(key);
  if (store_value != NULL) {
    if (store_value->type == STORE_STRING_TYPE) {
      if (store_value->content.string_value != NULL)
        free(store_value->content.string_value);
    }
    store_free_value(store_value);
    store_put(key, NULL);
  }
}

/* num digits */
int numDigits(int n) {
  int i = 0;
  while (n > 0) {
    n = n / 10;
    i++;
  }
  return i;
}
size_t str_list_len(char** list) {
  size_t i = 0;
  while (list[i] != NULL) {
    i += strlen(list[i]);
  }
  return i;
}
size_t list_len(char** list) {
  size_t i = 0;
  while (list[i] != NULL) {
    i++;
  }
  return i;
}

/*
 * calculate needed buffer size for watcher
 */
size_t watcher_buffer_size(WATCHER* watcher) {
  int size = 0;
  size += numDigits(watcher->id);
  size += strlen("\t");
  size += strlen(watcher->type->name);
  size += strlen("(");
  size += numDigits(watcher->pid);
  size += numDigits(watcher->read);
  size += numDigits(watcher->write);
  size += strlen(")");
  size += strlen("\n");
  return (size * sizeof(char) + str_list_len(watcher->type->argv));
}

/*
 * free wp args array
 */
void free_wp_args(char** args) {
  int i = 0;
  while (args[i] != NULL) {
    free(args[i]);
    i++;
  }
  free(args);
}

/*
 * create watcher_table string
 */
char* create_watcher_table() {
  WATCHER* current_watcher = watcher_table->head;
  if (current_watcher == NULL) {
    return NULL;
  }

  // init memstream
  char* table_buf;
  size_t table_buf_size;
  FILE* table_fd = open_memstream(&table_buf, &table_buf_size);
  if (table_fd == NULL) {
    // Handle the error here, for example by logging an error message
    // and exiting the program
    debug("Failed to allocate memory for memstream\n");
    exit(EXIT_FAILURE);
  }
  // char buf[1024];
  // memset(buf, 0, sizeof(buf));
  // ssize_t nbytes = 0;

  // handle rest of list
  while (current_watcher != NULL) {
    fprintf(table_fd, "%d\t%s(%d,%d,%d)", current_watcher->id,
            current_watcher->type->name, current_watcher->pid,
            current_watcher->read, current_watcher->write);
    if (current_watcher->type->argv != NULL) {
      fprintf(table_fd, " ");
      for (int i = 0; current_watcher->type->argv[i] != NULL; i++) {
        fprintf(table_fd, "%s ", current_watcher->type->argv[i]);
      }
      if (current_watcher->args != NULL) {
        fprintf(table_fd, "[");
        // debug("args: %s\n", current_watcher->args[0]);
        for (int i = 0; current_watcher->args[i] != NULL; i++) {
          fprintf(table_fd, "%s", current_watcher->args[i]);
          if (current_watcher->args[i + 1] != NULL) {
            fprintf(table_fd, " ");
          }
        }
        fprintf(table_fd, "]");
      }
    }
    fprintf(table_fd, "\n");
    current_watcher = current_watcher->next;
  }
  fflush(table_fd);
  fclose(table_fd);
  return table_buf;
}

void print_watcher_table(WATCHER* cli_watcher) {
  char* buffer = create_watcher_table();
  // printf("%s", buffer);
  cli_watcher->type->send(cli_watcher, buffer);
  if (buffer != NULL) {
    free(buffer);
  }
  return;
}

/*
 * Destroy all processes
 */
void destroy_all_processes() {
  WATCHER* current_watcher = watcher_table->head;
  while (current_watcher != NULL) {
    WATCHER* next = current_watcher->next;
    current_watcher->type->stop(current_watcher);
    // process to dispose of watcher
    dispose_of_parent(current_watcher);
    current_watcher = next;
  }
  free_available_ids();
  free(watcher_table);
}

/*
 * Destroy a watcher
 */
void destroy_watcher(int id) {
  WATCHER* watcher = get_watcher(id);
  if (watcher == NULL) {
    debug("watcher not found");
    return;
  }
  // kill the process
  watcher->type->stop(watcher);
  // remove the watcher from the list
  // remove_watcher(watcher);
  return;
}

void free_store_path(char* path) {
  remove_data(path);
  free(path);
}

/*
 disposes of a parent 🧼🚿
 */
void dispose_of_parent(WATCHER* wp) {
  // close the write end of the parent_to_child_pipe
  close(wp->write);
  // close the read end of the child_to_parent_pipe
  close(wp->read);

  if (wp->price_path != NULL) free_store_path(wp->price_path);
  if (wp->volume_path != NULL) free_store_path(wp->volume_path);

  free_id(wp->id);
  if (wp->args != NULL) free_wp_args(wp->args);
  if (wp->buffer != NULL) free(wp->buffer);
  remove_watcher(wp);
  free(wp);
}

/*
 * waits until a child is killed to kill parent too
 */
int wait_for_dead_child() {
  int status;
  pid_t pid;
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    if (pid == -1) {
      error("waitpid error");
      return -1;
    }
    WATCHER* wp = get_watcher_by_pid(pid);
    warn("CHILD REAPED %d", wp->id);
    dispose_of_parent(wp);
  }
  return 0;
}

/*
 * trace a program input
 */
void trace_program(WATCHER* wp, char* txt) {
  struct timespec current_time;
  clock_gettime(CLOCK_REALTIME, &current_time);
  // microseconds
  int ms = current_time.tv_nsec / 1000;

  // init memstream
  char* message_buf;
  size_t message_buf_size;
  FILE* message_fd = open_memstream(&message_buf, &message_buf_size);
  if (message_fd == NULL) {
    // Handle the error here, for example by logging an error message
    // and exiting the program
    debug("Failed to allocate memory for memstream\n");
    exit(EXIT_FAILURE);
  }
  fprintf(message_fd, "[%010ld.%06d][%s][%d][%d]: ", current_time.tv_sec, ms,
          wp->type->name, wp->read, wp->serial);
  fprintf(message_fd, "%s\n", txt);
  fflush(message_fd);
  fclose(message_fd);

  // send to stderr
  write(STDERR_FILENO, message_buf, strlen(message_buf));
  free(message_buf);
}

int ends_with_price(const char* str) {
  const char* suffix = ":price";
  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);
  if (str_len < suffix_len) {
    return 0;
  }
  return strncmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
}

int ends_with_volume(const char* str) {
  const char* suffix = ":volume";
  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);
  if (str_len < suffix_len) {
    return 0;
  }
  return strncmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
}