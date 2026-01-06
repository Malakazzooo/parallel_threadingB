# parallel_threadingB 

Part B: Concurrent Image Processing

Evolution of the application to support interactivity and simultaneous concurrency.

Non-Blocking Interactivity:
A Main Thread handles user inputs (DIR, STAT, QUIT) while Worker Threads process images in the background.

IPC Communication:
Implementation of Pipes to send processing tasks.

Synchronization:
Use of Mutexes (Pthreads) to protect access to global variables and statistics.