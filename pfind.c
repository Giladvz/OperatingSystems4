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
}thead;

typedef struct directoryHead {
    directoryNode * head;
    directoryNode * tail;
}dhead;

dhead *directoryList;
thead *threadSleepList;
pthread_t * threads;
pthread_mutex_t dirMutex;
pthread_mutex_t threadMutex;
pthread_cond_t * cvArray;
pthread_cond_t start;
atomic_int sleeping = 0;
atomic_int dead = 0;
int threadNum;
int firstRun = 0;

thead * create_sleep_list(void) {
    thead * head = (thead*)malloc(sizeof(thead));
    if(head == NULL) {
        perror("Error with malloc");
        exit(1);
    }
    head -> head = NULL;
    head -> tail = NULL;
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
    return head;
}

int add_directory_node(dhead * head, char * fullPath) {
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
    return 1;
}

void remove_directory_node(dhead * head) {
    directoryNode * tmp = head -> head;
    if (head -> head == head -> tail) {
        head -> head = NULL;
        head -> tail = NULL;
        free(tmp);
        return;
    }

    head -> head = tmp -> prev;
    head -> head -> next = NULL;
    free(tmp);
}

int isDirQEmpty(dhead * head){
    if (head->head == NULL) {
        return 1;
    }
    return 0;
}

void * startDirCheck(void* num) {
    long my_id = (long) num;
    char *path[PATH_MAX];
    sleeping++;
    pthread_cond_wait(&start,&threadMutex);
    sleeping--;
    //pthread_mutex_lock(dirMutex);

}
    /*
    // Fist run, 1 gets the file, the rest go to sleep
    if (firstRun) {
        // Add to thread sleeping list
        pthread_mutex_lock(threadMutex);
        add_thread_node(threadSleepList,my_id);
        pthread_mutex_unlock(threadMutex);
        if (threadSleepList -> head -> id == my_id) {
            pthread_cond_wait(isHeadCV);
        }
        else {
            pthread_cond_wait(isNotHeadCV);
        }
    }
    else {
        firstRun = 1;
        strcpy(path, directoryList->head->path);
        remove_directory_node(directoryList);
        pthread_mutex_unlock(dirMutex);
        searchDir(path);
    }
    while (isEveryOneWorking){
        while (!isDirQEmpty()){
            //searchDir()
        }
    }
    return 0;
}
*/
int main(int argc, char* argv[]) {
    long i;

    pthread_mutex_init(&dirMutex, NULL);
    pthread_mutex_init(&threadMutex, NULL);
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
    if(add_directory_node(directoryList,argv[1]) == 0 ) {
        exit(1);
    }

    pthread_t * threads = (pthread_t *)calloc(threadNum,sizeof(pthread_t));
    for (i=0; i<threadNum;i++){
        pthread_cond_init(&cvArray[i],NULL);
        if ((pthread_create(&threads[i], NULL, startDirCheck, (void *)i)) < 0) {
            perror("Error creating threads");
            exit(1);
        }
    }
    while(sleeping != threadNum) {sleep(1);}
    pthread_cond_broadcast(&start);

    for (int i = 0; i < threadNum; i++) {
        pthread_join(threads[i], NULL);
    }
    // Clean up and exit
    pthread_mutex_destroy(&dirMutex);
    pthread_mutex_destroy(&threadMutex);
    for (i =0; i<threadNum; i++) {
        pthread_cond_destroy(&cvArray[i]);
    }
}