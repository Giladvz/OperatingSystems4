#include <stdatomic.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

typedef struct threadNode {
    int id;
    struct threadNode * next;
    struct threadNode * prev;

}threadNode;

typedef struct directoryNode {
    char path[PATH_MAX];
    struct directoryNode * next;
    struct directoryNode * prev;

} directoryNode;

typedef struct threadHead {
    threadNode * head;
    threadNode * tail;
    int size;
}thead;

typedef struct directoryHead {
    directoryNode * head;
    directoryNode * tail;
    int size;
}dhead;

dhead *directoryList;
thead *threadSleepList;
pthread_t * threads;
pthread_mutex_t startMutex;
pthread_mutex_t dirMutex;
pthread_mutex_t threadMutex;
pthread_mutex_t removeMutex;
pthread_mutex_t finishMutex;
pthread_cond_t * cvArray;
pthread_cond_t start;
atomic_int sleeping = 0;
atomic_int dead = 0;
int threadNum;
int firstRun = 0;
int exitcode = 0;

thead * create_sleep_list(void) {
    thead * head = (thead*)malloc(sizeof(thead));
    if(head == NULL) {
        perror("Error with malloc");
        exit(1);
    }
    head -> head = NULL;
    head -> tail = NULL;
    head -> size = 0;
    return head;
}

dhead * create_directory_list(void) {
    dhead * head = (dhead*)malloc(sizeof(dhead));
    if(head == NULL) {
        perror("Error with malloc");
        exit(1);
    }
    head -> head = NULL;
    head -> tail = NULL;
    head -> size = 0;
    return head;
}

int isDirQEmpty(dhead * head){
    if (head->size == 0) {
        return 1;
    }
    return 0;
}

int isThreadQEmpty(thead * head){
    if (head -> size == 0) {
        return 1;
    }
    return 0;
}

int add_directory_node(dhead * head, char * fullPath, thead * threadSleepList) {
    directoryNode * new_node = (directoryNode *)malloc(sizeof(directoryNode));
    if (new_node == NULL) {
        perror("Error with malloc");
        return 0;
    }
    directoryNode * tmp = head -> tail;
    if (tmp != NULL) {
        tmp -> prev = new_node;
    }
    head -> tail = new_node;
    if (head -> head == NULL) {
        head -> head = new_node;
    }
    new_node -> next = tmp;
    strcpy(new_node -> path,fullPath);
    new_node -> prev = NULL;
    if (!isThreadQEmpty(threadSleepList)) {
        pthread_cond_signal(&cvArray[threadSleepList->head->id]);
    }
    head -> size +=1;
    return 1;
}

int add_thread_node(thead * head,int id){
    threadNode * new_node = (threadNode *)malloc(sizeof(threadNode));
    if (new_node == NULL) {
        perror("Error with malloc");
        return 0;
    }
    threadNode * tmp = head -> tail;
    if (tmp != NULL) {
        tmp -> prev = new_node;
    }
    head -> tail = new_node;
    if (head -> head == NULL) {
        head -> head = new_node;
    }
    new_node -> next = tmp;
    new_node -> prev = NULL;
    new_node -> id = id;
    head -> size +=1;
    return 1;
}

int getThreadLocation(thead * head,int num) {
    int loc = 0;
    threadNode * node = head -> head;
    while ((node -> id) != num ){
        node = node -> prev;
        loc++;
    }
    return loc;
}

char * remove_directory_node(dhead * head,int place) {
    directoryNode * tmp = head->head;
    head -> size -=1;
    while (place > 0){
        tmp = tmp -> prev;
    }
    if (head -> size == 0) {
        head -> head = NULL;
        head -> tail = NULL;
    }
    else if (tmp == head -> head) {
        head -> head = tmp -> prev;
        head -> head -> next = NULL;
    }
    else if (tmp == head -> tail) {
        tmp -> next -> prev =NULL;
        head -> tail = tmp -> next;
    }
    return tmp -> path;
}


int removeThreadNode(thead * head, int place) {
    threadNode * tmp = head ->head;
    head -> size -=1;
    while (place > 0){
        tmp = tmp -> prev;
    }
    if (head -> size == 0) {
        head -> head = NULL;
        head -> tail = NULL;
    }
    else if (tmp == head -> head) {
        head -> head = tmp -> prev;
        head -> head -> next = NULL;
    }
    else if (tmp == head -> tail) {
        tmp -> next -> prev =NULL;
        head -> tail = tmp -> next;
    }
    return tmp -> id;
}

void searchDir(char * path) {

}

void * startDirCheck(void* num) {
    long my_id = (long) num;
    char path[PATH_MAX];
    int loc;

    sleeping++;
    pthread_cond_wait(&start,&threadMutex);
    sleeping--;
    pthread_mutex_unlock(&threadMutex);
    perror("going in");
    while ((dead + sleeping != threadNum) || (!isDirQEmpty(directoryList))) {
        pthread_mutex_lock(&threadMutex);
        add_thread_node(threadSleepList,my_id);
        perror("Adding thread to list");
        pthread_mutex_unlock(&threadMutex);
        pthread_mutex_lock(&dirMutex);
        if (isDirQEmpty(directoryList)) {
            sleeping++;
            perror("thread is on way of sleep");
            // Everyone is asleep and the directory folder is empty. Last thread starts exit process
            if (dead + sleeping == threadNum) {
                // Tell main to set other threads free
                perror("c");
                printf("MY id is %ld", my_id);
                loc = getThreadLocation(threadSleepList,my_id);
                perror("1");
                removeThreadNode(threadSleepList,loc);
                perror("2");
                pthread_mutex_unlock(&dirMutex);
                perror("3");
                pthread_cond_signal(&start);
                break;
            }
            perror("B");
            pthread_cond_wait(&cvArray[my_id],&dirMutex);

            // Need to break to exit properly. Checks if main set the thread free
            if (dead + sleeping == threadNum && isDirQEmpty(directoryList)) {
                loc = getThreadLocation(threadSleepList,my_id);
                pthread_mutex_unlock(&dirMutex);
                break;
            }
            sleeping--;
            pthread_mutex_unlock(&dirMutex);
            pthread_mutex_lock(&removeMutex);
            loc = getThreadLocation(threadSleepList,my_id);
            removeThreadNode(threadSleepList,loc);
            strcpy(path, remove_directory_node(directoryList,loc));
            pthread_mutex_unlock(&removeMutex);
            searchDir(path);
        }
        else {
            pthread_mutex_unlock(&dirMutex);
            pthread_mutex_lock(&removeMutex);
            loc = getThreadLocation(threadSleepList,my_id);
            if (loc > directoryList -> size){
                sleeping++;
                pthread_cond_wait(&cvArray[my_id],&removeMutex);
                loc = getThreadLocation(threadSleepList,my_id);
                if ((dead + sleeping == threadNum) && (isDirQEmpty(directoryList))) {
                    pthread_mutex_unlock(&removeMutex);
                    break;
                }
                sleeping--;
            }
            removeThreadNode(threadSleepList,loc);
            strcpy(path, remove_directory_node(directoryList,loc));
            pthread_mutex_unlock(&removeMutex);
            searchDir(path);
        }
    }
    pthread_exit(0);
}

int main(int argc, char* argv[]) {
    long i;
    pthread_mutex_init(&dirMutex, NULL);
    pthread_mutex_init(&startMutex,NULL);
    pthread_mutex_init(&threadMutex, NULL);
    pthread_mutex_init(&removeMutex,NULL);
    pthread_mutex_init(&finishMutex,NULL);
    pthread_cond_init(&start,NULL);
    if (argc != 4) {
        errno = EINVAL;
        perror("There should be 3 paramaters");
        exit(1);
    }
    threadNum = atoi(argv[3]);
    DIR * check_dir = opendir(argv[1]);
    if (check_dir == NULL) {
        perror("Root directory is invalid.");
        exit(1);
    }
    if(closedir(check_dir) <0 ) {
        perror("Error closing dir");
        exit(1);
    }

    // Creating queues
    if ((directoryList = create_directory_list()) == NULL) {
        exit(1);
    }
    if ((threadSleepList = create_sleep_list()) == NULL) {
        exit(1);
    }
    // Adding root directory to queue
    if(add_directory_node(directoryList,argv[1],threadSleepList) == 0 ) {
        exit(1);
    }

    threads = (pthread_t *)calloc(threadNum,sizeof(pthread_t));
    if(threads == NULL) {
        perror("Error with malloc");
        exit(1);
    }
    cvArray = (pthread_cond_t *)calloc(threadNum,sizeof(pthread_cond_t));
    if(threads == NULL) {
        perror("Error with malloc");
        exit(1);
    }

    for (i=0; i<threadNum;i++){
        pthread_cond_init(&cvArray[i],NULL);
        if ((pthread_create(&threads[i], NULL, startDirCheck, (void *)i)) < 0) {
            perror("Error creating threads");
            exit(1);
        }
    }
    while(sleeping != threadNum) {
        sleep(1);
    }
    pthread_cond_broadcast(&start);

    // Wait untill all threads are finished
    pthread_cond_wait(&start,&finishMutex);
    perror("starting free");
    // All threads are finished, sets them free so they can exit. only main changes threadsleeplist.
    for (i = 0; i < sleeping -1; i++){
        pthread_cond_signal(&cvArray[threadSleepList ->head->id]);
        printf("freeing %d", threadSleepList ->head->id);
        removeThreadNode(threadSleepList,0);
    }
    for (i = 0; i < threadNum; i++) {
        pthread_join(threads[i], NULL);
    }
    // Clean up and exit
    pthread_mutex_destroy(&startMutex);
    pthread_mutex_destroy(&dirMutex);
    pthread_mutex_destroy(&threadMutex);
    pthread_mutex_destroy(&removeMutex);
    pthread_mutex_destroy(&finishMutex);
    for (i =0; i<threadNum; i++) {
        pthread_cond_destroy(&cvArray[i]);
    }
    exit(exitcode);
}