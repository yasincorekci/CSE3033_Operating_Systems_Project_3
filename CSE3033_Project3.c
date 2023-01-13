#include "CSE3033_Project3.h"

FILE *file;
ts_Queue *Header = NULL;
int num_lines;
void *readThreadFunc(void *arg) 
{
    long tid = (long)arg;
    int line_num;
    char line[MAX_LINE_LENGTH];
    char temp[MAX_LINE_LENGTH];
    int size;
    for(;;)
    {
        memset(temp, 0, sizeof(temp));
        pthread_mutex_lock(&read_mutex);
        line_num = num_lines++;
        pthread_mutex_lock(&write_mutex);
        long start_pos= ftell(file);
        if(fgets(line, MAX_LINE_LENGTH, file)!=NULL)
        {
            long end_pos = ftell(file);
            pthread_mutex_unlock(&read_mutex);
            pthread_mutex_unlock(&write_mutex);
            for(int i = 0; i < strlen(line); i++)
            {
                if(line[i] != '\n')
                {
                    temp[i] = line[i];
                }
            }
            ts_Queue* currentNode = addLine(line_num, temp);
            currentNode->end_pos=end_pos;
            currentNode->start_pos=start_pos;
            printf("Read_%ld  \t\tRead_%ld read the line %d which is %s \n",tid,tid,line_num,temp); 
            sem_post(&sem_upper);//Say to upper_func i read a line and you can upper a line
            sem_post(&sem_replace);//Say to replace_func i read a line and you can upper a line
            usleep(100);
        }
        else
        {
            pthread_mutex_unlock(&write_mutex);
            pthread_mutex_unlock(&read_mutex);
            sem_post(&sem_upper);
            sem_post(&sem_replace);
            num_lines--;
            return NULL;
        }
    }
}
void *upperThreadFunc(void *arg) 
{
    long tid = (long)arg;
    char temp[MAX_LINE_LENGTH];
    for(;;)
    {
        sem_wait(&sem_upper);
        pthread_mutex_lock(&upper_mutex);
        ts_Queue* currentNode = getNextAvailableLine(upperJob);
        pthread_mutex_unlock(&upper_mutex);
        if(currentNode==NULL)
        {
            sem_post(&sem_write);
            sem_post(&sem_upper);
            return NULL;
        }
        strcpy(temp,currentNode->msg);
        if(currentNode->isUpper_done==0)
        {
            for(int i=0;i<strlen(temp);i++)
            {
                currentNode->msg[i]=toupper(temp[i]);
            }
            printf("Upper_%ld  \t\tUpper_%ld read index %d and converted “%s” to “%s”\n",tid,tid,currentNode->line,temp,currentNode->msg);
            currentNode->isUpper_done=1;
            if(currentNode->isReplace_done==1)
            {
                sem_post(&sem_write);
            }
        }
        pthread_mutex_unlock(&currentNode->mutex);
        usleep(100);
    }
    return NULL;
}
void *replaceThreadFunc(void *arg) 
{
    long tid = (long)arg;
    char temp[MAX_LINE_LENGTH];
    for(;;)
    {
        sem_wait(&sem_replace);
        pthread_mutex_lock(&replace_mutex);
        ts_Queue* currentNode = getNextAvailableLine(replaceJob);
        pthread_mutex_unlock(&replace_mutex);
        if(currentNode==NULL)
        {
            sem_post(&sem_write);
            sem_post(&sem_replace);
            return NULL;
        }
        strcpy(temp,currentNode->msg);
        for(int i = 0; i < strlen(temp); i++)
        {
            if(temp[i]==' ')
            {
                currentNode->msg[i]='_';
            }
        }
        printf("Replace_%ld\t\tReplace_%ld read index %d and converted “%s” to “%s”\n",tid,tid,currentNode->line,temp,currentNode->msg);
        currentNode->isReplace_done=1;
        if(currentNode->isUpper_done==1)
        {
            sem_post(&sem_write);
        }
        pthread_mutex_unlock(&currentNode->mutex);
        usleep(100);
    }
    
    return NULL;
}
void *writeThreadFunc(void *arg) 
{
    long tid = (long)arg;
    int size;
    for(;;)
    {
        sem_wait(&sem_write);
        pthread_mutex_lock(&write_mutex);
        ts_Queue* currentNode = getNextAvailableLine(writeJob);
        pthread_mutex_unlock(&write_mutex);
        if(currentNode == NULL)//Tüm node'lar bittiyse
        {
            sem_post(&sem_write);
            return NULL;
        }
        printf("Write_%ld  \t\tWrite_%ld line %d back which “%s”\n",tid,tid,currentNode->line,currentNode->msg);
        long saved_pos = ftell(file);//Before changing the pointer of the file we store its position
        long start_pos =currentNode->start_pos;
        long end_pos =currentNode->end_pos;
        /* 
        In this are we change the pointer of the file, we process it and restore it, during this process,
        we put a lock so that the read thread does not enter when the pointer is in a
        different place than the next line that the read thread will read. 
        */
        pthread_mutex_lock(&file_mutex);
        fseek(file, start_pos, SEEK_SET);
        fprintf(file, "%s", currentNode->msg);
        fseek(file, saved_pos, SEEK_SET);
        pthread_mutex_unlock(&file_mutex);
        currentNode->isWrite_done = 1;
        pthread_mutex_unlock(&currentNode->mutex);
        usleep(100);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 8) 
    {
        printf("Wrong Inputs\n");
        return 1;
    }
    char fileName[32];
    strcpy(fileName, argv[2]);
    int nmbOfReadThreads = atoi(argv[4]);
    int nmbOfUpperThreads = atoi(argv[5]);
    int nmbOfReplaceThreads = atoi(argv[6]);
    int nmbOfWriteThreads = atoi(argv[7]);
    int nmbOfAllThreads=nmbOfReadThreads+nmbOfUpperThreads+nmbOfReplaceThreads+nmbOfWriteThreads;
    file = fopen(fileName, "r+");
    
    pthread_mutex_init(&read_mutex,NULL);
    pthread_mutex_init(&upper_mutex,NULL);
    pthread_mutex_init(&replace_mutex,NULL);
    pthread_mutex_init(&file_mutex, NULL);
    sem_init(&sem_upper,0,0);
    sem_init(&sem_replace,0,0);
    sem_init(&sem_write,0,0);
    long i;
    pthread_t threads[nmbOfAllThreads];
    printf("<Thread-type and ID>\t<Output>\n");
    for (i = 1; i <=nmbOfAllThreads; i++) 
    {
        if(i<=nmbOfReadThreads)
        {
            pthread_create(&threads[i], NULL, readThreadFunc, (void*)i);
        }
        else if(i>=nmbOfReadThreads && i<=nmbOfUpperThreads+nmbOfReadThreads)
        {
            pthread_create(&threads[i], NULL, upperThreadFunc, (void*)i-nmbOfReadThreads);
        }
        else if(i>=nmbOfReadThreads+nmbOfUpperThreads && i<=nmbOfUpperThreads+nmbOfReadThreads+nmbOfReplaceThreads)
        {
            pthread_create(&threads[i], NULL, replaceThreadFunc, (void*)i-nmbOfReadThreads-nmbOfUpperThreads);
        }
        else if(i>=nmbOfReadThreads+nmbOfUpperThreads+nmbOfReplaceThreads && i<=nmbOfUpperThreads+nmbOfReadThreads+nmbOfReplaceThreads+nmbOfWriteThreads)
        {
            pthread_create(&threads[i], NULL, writeThreadFunc, (void*)i-nmbOfReadThreads-nmbOfUpperThreads-nmbOfReplaceThreads);
        }
    }
    for (i = 1; i <= nmbOfAllThreads; i++) 
    {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&replace_mutex);
    pthread_mutex_destroy(&read_mutex);
    pthread_mutex_destroy(&upper_mutex);
    pthread_mutex_destroy(&file_mutex);
    sem_destroy(&sem_upper);
    sem_destroy(&sem_replace);
    sem_destroy(&sem_write);
    fclose(file);
    printf("Program Finished Succesfly\n");
    // displayQueue();
    return 0;
}
ts_Queue* addLine(int newLine,char *line) 
{
	ts_Queue* newNode = (ts_Queue*)malloc(sizeof(ts_Queue));
    newNode->line = newLine;
    newNode->msg = (char*)malloc(strlen(line) + 1);
    strcpy(newNode->msg, line);
	pthread_mutex_init(&newNode->mutex, NULL);
	ts_Queue *currentNode = Header;
	if(Header==NULL)
	{
		Header=newNode;
		return newNode;
	}
	while (currentNode->nextLine != NULL) 
	{
		currentNode = (ts_Queue*)currentNode->nextLine;
        
	}
	currentNode->nextLine = (struct ts_Queue*) newNode;
    newNode->previusLine=(struct ts_Queue*) currentNode;
    
	return newNode;
}
ts_Queue* displayQueue()
{
	ts_Queue* currentNode=Header;
	while(currentNode!=NULL)
	{
		printf("Line Number is :%d     msg=%s\n",currentNode->line,currentNode->msg);
		currentNode=(ts_Queue*)currentNode->nextLine;
	}
	return NULL;
}
ts_Queue* getNextAvailableLine(int job)
{
    for(;;)
    {
        ts_Queue* currentNode=Header;
        while(currentNode!=NULL)
        {
            if(pthread_mutex_trylock(&currentNode->mutex)==0)
            {

                if(job==upperJob && currentNode->isUpper_done==0)
                {
                    return currentNode;
                }
                else if(job==replaceJob && currentNode->isReplace_done==0)
                {
                    return currentNode;
                }
                else if(job==writeJob && currentNode->isWrite_done==0 && currentNode->isReplace_done==1 && currentNode->isUpper_done==1)
                {
                    return currentNode;
                }
                else
                {
                    pthread_mutex_unlock(&currentNode->mutex);
                }
            }
            currentNode=(ts_Queue*)currentNode->nextLine;
        }
        if(threadAllDone()==1)
        {
            return NULL;
        }
    }
}
int threadAllDone()
{
    ts_Queue* currentNode=Header;
    int temp=0;
	while(currentNode!=NULL)
	{
        if(currentNode->isUpper_done==1 && currentNode->isReplace_done==1 && currentNode->isWrite_done==1)
        {
        }
        else
        {
            return 0;
        }
        currentNode=(ts_Queue*)currentNode->nextLine;
    }
    return 1;
}

