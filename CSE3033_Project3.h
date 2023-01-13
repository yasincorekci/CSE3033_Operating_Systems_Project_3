#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_LINES 256
#define MAX_LINE_LENGTH 256

typedef struct
{
    struct ts_Queue *previusLine;
    int line;
    long start_pos;
    long end_pos;
    char *msg;
    int isUpper_done;
    int isReplace_done;
    int isWrite_done;
    pthread_mutex_t mutex;
    struct ts_Queue *nextLine;
}ts_Queue;

typedef enum
{
    upperJob,
    replaceJob,
    writeJob
}te_Jobs;

ts_Queue* addLine(int newLine,char msg[MAX_LINE_LENGTH]);
ts_Queue* displayQueue();
ts_Queue* getNextAvailableLine(int job);
int chechAllDone();
int threadAllDone();
int deleteFromQueue(ts_Queue* currentNode);

sem_t sem_upper;
sem_t sem_replace;
sem_t sem_write;

pthread_mutex_t read_mutex;
pthread_mutex_t upper_mutex;
pthread_mutex_t replace_mutex;
pthread_mutex_t write_mutex;
pthread_mutex_t file_mutex;


