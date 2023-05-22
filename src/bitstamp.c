#include <stdio.h>
#include <stdlib.h>

#include "argo.h"
#include "debug.h"
#include "helpers.h"
#include "ticker.h"
#include "watcher.h"

WATCHER *bitstamp_watcher_start(WATCHER_TYPE *type, char *args[]) {
  if (args == NULL) {
    debug("args is null");
    return NULL;
  }
  if (args[0] == NULL) {
    debug("args[0] is null");
    return NULL;
  }

  WATCHER *bitstmp = malloc(sizeof(WATCHER));
  bitstmp->id = get_available_id();
  bitstmp->serial = 0;
  bitstmp->args = args;

  // debug("args[0]: %s\n", bitstmp->args[0]);
  bitstmp->type = type;
  bitstmp->trace = 0;
  bitstmp->next = NULL;
  bitstmp->buffer = NULL;

  // get name
  char *watcher_name = bitstmp->type->name;
  char *xxx = bitstmp->args[0];
  char *priceStr = ":price";
  char *vol = ":volume";
  char *price_key = malloc((strlen(watcher_name) + strlen(xxx) +
                            strlen(priceStr) + strlen(vol) + 2) *
                           sizeof(char));
  sprintf(price_key, "%s:%s%s", watcher_name, xxx, priceStr);
  char *vol_key = malloc((strlen(watcher_name) + strlen(xxx) +
                          strlen(priceStr) + strlen(vol) + 2) *
                         sizeof(char));
  sprintf(vol_key, "%s:%s%s", watcher_name, xxx, vol);
  bitstmp->price_path = price_key;
  // add_double_data(bitstmp->price_path, 0.0);
  bitstmp->volume_path = vol_key;
  // add_double_data(bitstmp->volume_path, 0.0);

  add_watcher(bitstmp);

  // create two pipes
  int read_fd_pipe[2], write_fd_pipe[2];
  if (pipe(read_fd_pipe) < 0 || pipe(write_fd_pipe) < 0) {
    debug("pipe err");
    return NULL;
  }

  // set parent pipe to be non-blocking

  // set watcher read and write
  bitstmp->read = read_fd_pipe[0];    // 3 / read to 3
  bitstmp->write = write_fd_pipe[1];  // 6 / write to 6

  debug("pipe created: %d, %d", read_fd_pipe[0], write_fd_pipe[1]);
  // create_fd_node();

  // fork the process
  pid_t pid = fork();
  if (pid == -1) {
    debug("fork err");
    return NULL;
  } else if (pid == 0) {
    // close the unused ends of the pipe
    // warn("close parent ends of the pipe, %d, %d", write_fd_pipe[1],
    //      read_fd_pipe[0]);

    close(write_fd_pipe[1]);  // close 3 / child does not read from parent
    close(read_fd_pipe[0]);   // close 6
    // child process
    if (dup2(write_fd_pipe[0], STDIN_FILENO) == -1) {
      // instead of stdin, parent should write to child
      debug("dup2 write pipe err");
      return NULL;
    }
    // redirect stdout to pipe
    if (dup2(read_fd_pipe[1], STDOUT_FILENO) == -1) {
      // instead of stdout, child should write to parent
      debug("dup2 read pipe err");
      return NULL;
    }

    // execute the command
    execvp(bitstmp->type->argv[0], bitstmp->type->argv);
    debug("execvp err");
    return NULL;
  } else {
    // parent process

    // save process id
    // debug("parent process id: %d", pid);
    bitstmp->pid = pid;

    // close child ends of the pipe
    // warn("close child ends of the pipe, %d, %d", write_fd_pipe[0],
    //      read_fd_pipe[1]);
    close(write_fd_pipe[0]);  // close 3 / child does not read from parent
    close(read_fd_pipe[1]);   // close 6

    // get I/O notification on stdout
    fcntl(read_fd_pipe[0], F_SETFL, O_ASYNC | O_NONBLOCK);
    // getpid to receive the SIGIO signal for stdout
    fcntl(read_fd_pipe[0], F_SETOWN, getpid());
    // set the signal to be sent as the asynchronous I/O notification for stdout
    fcntl(read_fd_pipe[0], F_SETSIG, SIGIO);

    // char count
    int count = 69 + strlen(bitstmp->args[0]);
    char message[count];
    // write to the write end of the pipe to send input to the child process
    sprintf(
        message,
        "{ \"event\": \"bts:subscribe\", \"data\": { \"channel\": \"%s\" } }\n",
        args[0]);
    bitstmp->type->send(bitstmp, message);
    // wait for the child process to finish
  }
  return bitstmp;
}

int bitstamp_watcher_stop(WATCHER *wp) {
  pid_t pid = wp->pid;
  // send stop message
  kill(pid, SIGTERM);
  return 0;
}

int bitstamp_watcher_send(WATCHER *wp, void *arg) {
  write(wp->write, arg, strlen(arg));
  return 0;
}

int bitstamp_watcher_recv(WATCHER *wp, char *txt) {
  wp->serial++;
  if ((wp->trace)) {
    trace_program(wp, txt);
  }
  // debug("bitstamp_watcher_recv: %s", txt);
  char *expected = "> \b\bServer message: '";
  // info("recieved: %s", txt);

  // compare text to expected
  // read through text character by character
  int i = 0;
  while (i < strlen(expected) && i < strlen(txt)) {
    if (txt[i] != expected[i]) {
      // error("failed on %d, text[i]=%c, expected[i]=%c", i, txt[i],
      // expected[i]);
      return 1;
    }
    i++;
  }
  // 2 backspaces
  // 1 newline

  // turn the text after : into an input stream
  if (strlen(txt) < strlen(expected)) {
    // error("strlen(txt) < strlen(expected)");
    return 1;
  }

  char *json_buffer =
      malloc(((strlen(txt) + 1) - strlen(expected)) * sizeof(char));
  strcpy(json_buffer, txt + strlen(expected));

  // create a new input stream associated with the json buffer
  FILE *json_stream =
      fmemopen(json_buffer, (strlen(txt) - strlen(expected)), "r");
  if (json_stream == NULL) {
    // if the input stream cannot be created, then return 1
    free(json_buffer);
    return 1;
  }

  // read the input stream as a JSON value
  ARGO_VALUE *json_argo_v = argo_read_value(json_stream);
  
  fclose(json_stream);
  free(json_buffer);

  if (json_argo_v == NULL) {
    // if the text is not valid JSON code, then return 1
    // debug("json_argo_v is null");
    return 1;
  }

  // json object!!! success :D
  // if the text is valid JSON code, then attempt to interpret it as a JSON
  // object
  ARGO_VALUE *event_field = argo_value_get_member(json_argo_v, "event");

  if (event_field == NULL) {
    // if the JSON object does not have a member named "event", then return 1
    error("i couldnt find an event field :((");
    argo_free_value(json_argo_v);
    return 1;
  }

  // debug("got event field");
  // confirm that event is "trade"

  // get "trade" value from ARGO_OBJECT
  char *event = argo_value_get_chars(event_field);
  if (strcmp(event, "trade") != 0) {
    // if the value of "event" is not "trade", then return 1
    // info("event was not a trade event");
    argo_free_value(json_argo_v);
    free(event);
    return 1;
  }
  free(event);

  // debug("got trade event");

  ARGO_VALUE *data = argo_value_get_member(json_argo_v, "data");
  if (data == NULL) {
    error("no data field");
    // if the JSON object does not have a member named "data", then return 1
    argo_free_value(json_argo_v);
    return 1;
  }

  // debug("got data field");

  // get "price" and "amount" values from data ARGO_OBJECT

  // get "price field" value from ARGO_OBJECT
  ARGO_VALUE *price_field = argo_value_get_member(data, "price");
  if (price_field == NULL) {
    error("no price field");
    argo_free_value(json_argo_v);
    // if the JSON object does not have a member named "data", then return 1
    return 1;
  }

  // get "price" value from ARGO_OBJECT
  double price = 0;
  if (argo_value_get_double(price_field, &price) != 0) {
    // if the value of "price" is not an integer, then return 1
    error("price was not an double");
    argo_free_value(json_argo_v);
    return 1;
  }
  // debug("got price field");

  // get "amount field" value from ARGO_OBJECT
  ARGO_VALUE *amount_field = argo_value_get_member(data, "amount");
  if (amount_field == NULL) {
    error("no amount field");
    argo_free_value(json_argo_v);
    // if the JSON object does not have a member named "data", then return 1
    return 1;
  }

  double amount = 0;
  if (argo_value_get_double(amount_field, &amount) != 0) {
    // if the value of "amount" is not an double, then return 1
    error("amount was not an double");
    argo_free_value(json_argo_v);
    return 1;
  }
  // debug("got amount field");
  argo_free_value(json_argo_v);

  // add data to the data store
  add_double_data(wp->price_path, price);
  // debug("price key: %s", price_key);
  // debug("volume key: %s", vol_key);

  // update volume data from data store
  double old_vol = get_double_data(wp->volume_path);
  if (old_vol == -1.0) {
    add_double_data(wp->volume_path, amount);
  } else {
    old_vol += amount;
    add_double_data(wp->volume_path, old_vol);
  }
  return 0;
}

int bitstamp_watcher_trace(WATCHER *wp, int enable) {
  wp->trace = enable;
  return 0;
}
