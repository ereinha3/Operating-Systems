# Project 2: Process Scheduling Implementation

This project implements various process scheduling algorithms in C, demonstrating core operating system concepts related to process management and scheduling.

## Project Structure

- `part1.c`: Basic process creation and management
- `part2.c`: First-Come-First-Serve (FCFS) scheduling
- `part3.c`: Round Robin scheduling
- `part4.c`: Priority scheduling
- `part5.c`: Multi-level feedback queue
- `cpubound.c`: CPU-bound process simulation
- `iobound.c`: I/O-bound process simulation
- `Makefile`: Build configuration

## Test Programs

- `cpubound.c`: Simulates CPU-intensive processes
- `iobound.c`: Simulates I/O-intensive processes
- `input.txt`: Configuration file for testing

## Implementation Details

The project is divided into five parts, each implementing different scheduling algorithms:
1. Basic Process Management
2. FCFS Scheduling
3. Round Robin Scheduling
4. Priority-based Scheduling
5. Multi-level Feedback Queue

## Building and Running

To compile all parts:
```bash
make all
```

To run individual parts:
```bash
./part1
./part2
./part3
./part4
./part5
```

## Documentation and Results

- `Project_2_Description.pdf`: Original project requirements
- `Screenshots/`: Directory containing execution results
- `logs/`: Directory containing execution logs

## Performance Analysis

Each scheduling algorithm has been tested with both CPU-bound and I/O-bound processes to analyze:
- Throughput
- Response time
- Fairness
- Resource utilization

Detailed results and comparisons can be found in the Screenshots directory.
