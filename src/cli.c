#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "debug.h"
#include "helpers.h"
#include "store.h"
#include "ticker.h"
#include "watcher.h"

/*
 * start a cli watcher
 */
WATCHER *cli_watcher_start(WATCHER_TYPE *type, char *args[]) {
  WATCHER *cli = malloc(sizeof(WATCHER));

  // get I/O notification on stdin
  fcntl(STDIN_FILENO, F_SETFL, O_ASYNC | O_NONBLOCK);
  // getpid to receive the SIGIO signal for stdin
  fcntl(STDIN_FILENO, F_SETOWN, getpid());
  // set the signal to be sent as the asynchronous I/O notification for stdin
  fcntl(STDIN_FILENO, F_SETSIG, SIGIO);

  cli->id = 0;
  cli->pid = -1;
  cli->type = type;
  cli->args = NULL;
  cli->buffer = NULL;
  cli->write = STDOUT_FILENO;
  cli->read = STDIN_FILENO;
  cli->price_path = NULL;
  cli->volume_path = NULL;
  cli->serial = 0;
  cli->trace = 0;
  cli->next = NULL;
  cli->type->send(cli, "");
  return cli;
}

int cli_watcher_stop(WATCHER *wp) { return 0; }

int cli_watcher_send(WATCHER *wp, void *arg) {
  // cast the arg to a char*
  char *arg_char = (char *)arg;
  // send the command to write
  write(wp->write, arg_char, strlen(arg_char));
  // write a newline
  write(wp->write, "ticker> ", strlen("ticker> "));
  return 0;
}

int cli_watcher_recv(WATCHER *wp, char *txt) {
  // debug("command received: %s", command_buffer);
  // split the command up by spaces
  wp->serial++;
  char *command = strtok(txt, " ");
  // char* arg1 = strtok(NULL, " ");
  // debug("command parsed: %s", command);
  if (command == NULL) {
    wp->type->send(wp, "???\n");
    return 1;
  }
  if ((wp->trace)) {
    trace_program(wp, txt);
  }
  // implement switch case for 4 commands
  if (strcmp(command, "start") == 0) {
    char *type = strtok(NULL, " ");
    // check if type null
    if (type == NULL) {
      wp->type->send(wp, "???\n");
      return 1;
    }

    WATCHER_TYPE *watcher_type = find_watcher_type(type);

    if (watcher_type == NULL) {
      debug("type not found");
      wp->type->send(wp, "???\n");
      return 1;
    } else if (strcmp(watcher_type->name, "CLI") == 0) {
      debug("cannot create CLI watcher");
      wp->type->send(wp, "???\n");
      return 1;
    }

    int current_size = 100;
    int needed_size = 0;
    char **args = malloc(sizeof(char *) * current_size);

    char *single_arg = strtok(NULL, " ");
    // if (single_arg == NULL) {
    //   debug("no arguments");
    //   wp->type->send(wp, "???\n");
    //   free(args);
    //   return 1;
    // }

    while (single_arg != NULL) {
      if (needed_size >= current_size) {
        current_size *= 2;
        args = realloc(args, sizeof(char *) * current_size);
      }
      args[needed_size] = strdup(single_arg);
      single_arg = strtok(NULL, " ");
      needed_size++;
    }
    args[needed_size] = NULL;
    args = realloc(args, sizeof(char *) * (needed_size + 1));
    current_size = needed_size;

    if (watcher_type->start(watcher_type, args) == NULL) {
      debug("watcher start failed");
      wp->type->send(wp, "???\n");
      free(args);
      return 1;
    };
    // free(args);
    wp->type->send(wp, "");
  } else if (strcmp(command, "stop") == 0) {
    char *id = strtok(NULL, " ");
    // check if id null
    if (id == NULL) {
      debug("id null in stop");
      wp->type->send(wp, "???\n");
      return 1;
    }
    // check for extraneous arguments
    if (strtok(NULL, " ") != NULL) {
      debug("extraneous arguments in stop");
      wp->type->send(wp, "???\n");
      return 1;
    }
    // check if id is 0
    if (atoi(id) == 0) {
      debug("id 0 is not allowed");
      // wp->type->send(wp, "ERROR: Stopping CLI Watcher is not allowed");
      wp->type->send(wp, "???\n");
      return 1;
    }
    // check if id is assigned
    if (!id_is_assigned(atoi(id))) {
      debug("id is not assigned in trace");
      wp->type->send(wp, "???\n");
      return 1;
    }

    // the binding of isaac
    fcntl(get_watcher(atoi(id))->read, F_SETFL, 0);
    destroy_watcher(atoi(id));
    wp->type->send(wp, "");
  } else if (strcmp(command, "watchers") == 0) {
    // check for extraneous arguments
    if (strtok(NULL, " ") != NULL) {
      debug("extraneous arguments in watchers");
      wp->type->send(wp, "???\n");
      return 1;
    }
    print_watcher_table(wp);
  } else if (strcmp(command, "trace") == 0) {
    char *id = strtok(NULL, " ");
    // check if id null
    if (id == NULL) {
      debug("id is null in trace");
      wp->type->send(wp, "???\n");
      return 1;
    }
    // check for extraneous arguments
    if (strtok(NULL, " ") != NULL) {
      debug("extraneous arguments in trace");
      wp->type->send(wp, "???\n");
      return 1; 
    }
    if (!id_is_assigned(atoi(id))) {
      debug("id is not assigned in trace");
      wp->type->send(wp, "???\n");
      return 1;
    }
    get_watcher(atoi(id))->type->trace(get_watcher(atoi(id)), 1);
    wp->type->send(wp, "");
  } else if (strcmp(command, "untrace") == 0) {
    char *id = strtok(NULL, " ");
    // check if id is null
    if (id == NULL) {
      debug("id is null in untrace");
      wp->type->send(wp, "???\n");
      return 1;
    }
    if (!id_is_assigned(atoi(id))) {
      debug("id is not assigned in trace");
      wp->type->send(wp, "???\n");
      return 1;
    }
    // check for extraneous arguments
    if (strtok(NULL, " ") != NULL) {
      debug("extraneous arguments in untrace");
      wp->type->send(wp, "???\n");
      return 1;
    }
    get_watcher(atoi(id))->type->trace(get_watcher(atoi(id)), 0);
    wp->type->send(wp, "");
  } else if (strcmp(command, "quit") == 0) {
    // check for extraneous arguments
    if (strtok(NULL, " ") != NULL) {
      debug("extraneous arguments in untrace");
      wp->type->send(wp, "???\n");
      return 1;
    }
    wp->type->send(wp, "");
    close(wp->write);
    // send sigint signal
    kill(getpid(), SIGINT);
  } else if (strcmp(command, "show") == 0) {
    char *key = strtok(NULL, " ");
    
    // what is the key?
    debug("given key: '%s'", key);
    debug("expected key: '%s'", watcher_table->head->next->price_path );

    // check if key is null
    if (key == NULL) {
      debug("key is null in show");
      wp->type->send(wp, "???\n");
      return 1;
    }
    // check extraneous arguments
    if (strtok(NULL, " ") != NULL) {
      debug("extraneous arguments in show");
      wp->type->send(wp, "???\n");
      return 1;
    }
    double price = get_double_data(key);
    if (price == -1.0) {
      error("made it to price, bad key");
      // wp->type->send(wp, "ERROR: There is no data associated with this
      // key\n");
      wp->type->send(wp, "???\n");
      return 1;
    }
    // format new string with 6 decimal places
    char *new_string = malloc(sizeof(char) * (strlen(key) + 20));
    sprintf(new_string, "%s\t%.6f\n", key, price);
    wp->type->send(wp, new_string);
    free(new_string);
    return 1;

  } else {
    debug("did not understand: %s", command);
    wp->type->send(wp, "???\n");
  }
  return 0;
}

int cli_watcher_trace(WATCHER *wp, int enable) {   
  wp->trace = enable;
  return 0; 
}
