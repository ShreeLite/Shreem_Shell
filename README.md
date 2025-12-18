# Shreem_Shell

> **Note:** This code is ported from [https://github.com/CS3-OSN-Monsoon-2025/mini-project-1-ShreeLite](https://github.com/CS3-OSN-Monsoon-2025/mini-project-1-ShreeLite)  
> This project was originally developed by me as part of the Operating Systems and Networks course (Monsoon 2025) at IIIT Hyderabad.  
> **It is ported here to my personal GitHub for archival and portfolio purposes.**

## Overview

Shreem_Shell is a custom Unix shell implementation written in C that provides a command-line interface with support for built-in commands, process management, I/O redirection, piping, and background job control.

## Features

### Built-in Commands

- **`hop`** - Navigate directories (similar to `cd`)
- **`reveal`** - List directory contents with support for flags (`-a` for hidden files, `-l` for detailed listing)
- **`log`** - Command history management
- **`activities`** - Display all running and stopped background processes
- **`ping`** - Send signals to processes
- **`fg`** - Bring background jobs to foreground
- **`bg`** - Resume stopped background jobs
- **`pwd`** - Print current working directory
- **`exit`** - Exit the shell

### Advanced Features

- **I/O Redirection**: Support for input (`<`), output (`>`), and append (`>>`) redirection
- **Piping**: Chain multiple commands using pipes (`|`)
- **Background Processes**: Run commands in background using `&`
- **Process Control**: Handle `Ctrl-C` and `Ctrl-Z` for job control
- **Command Chaining**: Execute multiple commands sequentially using `;`
- **Job Management**: Track and manage background and stopped processes

## Project Structure

```
Shreem_Shell/
├── README.md
└── shell/
    ├── Makefile           # Build configuration
    ├── include/
    │   └── shell.h        # Header file with function prototypes and structures
    └── src/
        ├── main.c         # Main shell loop and command execution
        ├── tokeniser.c    # Input tokenization
        ├── parser.c       # Command parsing
        ├── prompt.c       # Shell prompt display
        ├── hop.c          # Directory navigation
        ├── reveal.c       # Directory listing
        ├── log.c          # Command history
        ├── activities.c   # Process listing
        ├── ping.c         # Signal sending
        ├── fg.c           # Foreground job control
        ├── bg.c           # Background job management
        ├── ctrl.c         # Signal handlers
        ├── input.c        # Input redirection
        ├── output.c       # Output redirection
        ├── pipe.c         # Pipe handling
        ├── seq.c          # Sequential command execution
        └── cat.c          # Additional utilities
```

## Building and Running

### Prerequisites

- GCC compiler
- Make utility
- Linux/Unix environment

### Build Instructions

```bash
cd shell
make
```

This will compile all source files and create the executable `shell.out`.

### Running the Shell

```bash
./shell.out
```

### Cleaning Build Files

```bash
make clean
```

## Usage Examples

```bash
# Navigate to a directory
hop /home/user/documents

# List files with details and hidden files
reveal -la

# Run a command in background
sleep 100 &

# View all background processes
activities

# Redirect output to a file
echo "Hello World" > output.txt

# Pipe commands
cat file.txt | grep "pattern" | wc -l

# Chain multiple commands
ls ; pwd ; echo "Done"

# Bring a background job to foreground
fg 1

# Send a signal to a process
ping 1234 9
```

## Technical Details

### Token Types
The shell recognizes the following token types:
- `TOKEN_NAME` - Command names and arguments
- `TOKEN_PIPE` - Pipe operator (`|`)
- `TOKEN_AMPERSAND` - Background operator (`&`)
- `TOKEN_SEMICOLON` - Command separator (`;`)
- `TOKEN_INPUT` - Input redirection (`<`)
- `TOKEN_OUTPUT` - Output redirection (`>`)
- `TOKEN_DOUBLE_OUTPUT` - Append redirection (`>>`)

### Process Management
- Supports up to 100 concurrent background jobs
- Tracks process states (running/stopped)
- Handles process cleanup and zombie prevention
- Implements proper signal handling for job control

## Implementation Highlights

- **Lexical Analysis**: Robust tokenization of input commands
- **Recursive Descent Parser**: Structured parsing of command syntax
- **Process Groups**: Proper management of process groups for job control
- **Signal Handling**: Custom handlers for `SIGINT`, `SIGTSTP`, and `SIGCHLD`
- **Memory Management**: Careful handling of buffers and dynamic data structures

## Course Information

- **Course**: Operating Systems and Networks
- **Semester**: Monsoon 2025
- **Institution**: IIIT Hyderabad
- **Original Repository**: [CS3-OSN-Monsoon-2025/mini-project-1-ShreeLite](https://github.com/CS3-OSN-Monsoon-2025/mini-project-1-ShreeLite)

## License

This project was created for educational purposes as part of coursework at IIIT Hyderabad.