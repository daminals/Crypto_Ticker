# Crypto_Ticker
An asynchronous web watcher utilizing signals to track cryptocurrencies

## Introduction

The Crypto Ticker is a web watcher program designed to monitor real-time data feeds from a cryptocurrency exchange. It manages a collection of concurrently executing "watcher" processes that gather information from data feeds and report events to the main ticker process. The program utilizes low-level Unix/POSIX system calls for process management, signal handling, and I/O redirection.

## Features

- Concurrently monitor real-time data feeds from a cryptocurrency exchange.
- Support for multiple watcher processes that report events to the ticker process.
- Command-line interface (CLI) for user interaction and control.
- Start, stop, and trace specific watcher instances.
- Data store for accumulating desired information from events.
- Signal handling for graceful termination and child process management.

## How it Works

The ticker program consists of the following components and functionalities:

1. Watcher Processes: The program manages multiple watcher processes that monitor real-time data feeds from the web. These watcher processes are responsible for receiving events from the data feeds and reporting them to the ticker process.
2. CLI Watcher: The command-line interface (CLI) watcher provides a prompt for user input and executes commands to control the program. It allows users to start new watcher instances, stop existing watchers (except for the CLI watcher itself), trace watcher instances, and view a list of active watchers.
3. Watcher Types: The program supports different watcher types, with the Bitstamp watcher being the primary implementation. Each watcher type has its own specific behavior and handles events from the corresponding data feed.
4. Data Store: The ticker process maintains a data store that accumulates desired information from the events received by the watcher processes. The data store associates values with specific keys, which can be queried using the "show" command.
5. Signal Handling: The program utilizes signal handling to gracefully terminate the ticker process and manage child processes. When a watcher process terminates, the program receives a SIGCHLD signal, allowing it to clean up resources associated with the terminated process.
Command Overview

The ticker program supports the following commands in the CLI:

- `quit`: Terminate the ticker program gracefully, including all active watcher processes.
- `watchers`: Print a list of active watchers, including their IDs, types, process IDs, and file descriptors.
- `start [watcher_type] [args...]`: Start a new watcher instance of the specified type with optional additional arguments.
- `stop [watcher_id]`: Stop the watcher instance with the given ID. The associated child process will be terminated.
- `trace [watcher_id]`: Enable tracing for the specified watcher instance. Traced input lines will be printed to the standard error output.
- `untrace [watcher_id]`: Disable tracing for the specified watcher instance.
- `show [key]`: Display the value associated with the specified key in the data store.

## Implementation Details

The ticker program uses processes, signals, and handlers to create and manage the watcher instances. It utilizes several system calls for process execution, I/O redirection, and child process termination. The bitstamp.net watcher type employs the uwsc program to establish and manage the Websocket connection to the server.

The program utilizes the fork() system call to create child processes for the watcher instances. It sets up pipes to redirect I/O between the main ticker process and the child processes using pipe(), dup2(), and close() system calls. Communication between the main process and child processes occurs through these pipes.

Signal handling is implemented using the sigaction() system call to handle the SIGCHLD signal, which indicates the termination of
a watcher process. The main ticker process uses waitpid() to determine which watcher processes have terminated, allowing it to clean up resources and manage the watcher table.
