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
    long id;
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
pthread_mutex_t mutex;
pthread_mutex_t addRemoveMutex;
pthread_mutex_t finishMutex;
pthread_cond_t * cvArray;
pthread_cond_t start;
atomic_int sleeping = 0;
atomic_int dead = 0;
int threadNum;
int dontStart = 0;
int exitcode = 0;
char searchTerm[PATH_MAX];
atomic_int numFound=0;

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

int getThreadLocation(thead * head,long num) {
    int loc = 0;
    threadNode * node = head -> head;
    while ((node -> id) != num ){
        node = node -> prev;
        loc++;
    }
    return loc;
}

threadNode * getThreadInIndex(thead * head,int index) {
    threadNode * tmp = head ->head;
    while (index > 0) {
        tmp = tmp ->prev;
        index--;
    }
    return tmp;
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
    head -> size +=1;
    if (threadSleepList -> size >= head -> size) {
        pthread_cond_signal(&cvArray[getThreadInIndex(threadSleepList,head ->size-1) -> id]);
    }
    return 1;
}

int add_thread_node(thead * head,long id){
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

char * remove_directory_node(dhead * head,int place) {
    directoryNode * tmp = head->head;
    head -> size -=1;
    while (place > 0){
        tmp = tmp -> prev;
        place--;
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
    else {
        tmp -> next -> prev = tmp -> prev;
        tmp -> prev -> next = tmp -> next;
    }
    return tmp -> path;
}


int removeThreadNode(thead * head, int place) {
    perror("ERROR IN remove thread????????????????????????????\n");
    threadNode * tmp = head ->head;
    head -> size -=1;
    while (place > 0){
        tmp = tmp -> prev;
        place--;
    }
    if (head -> size == 0) {
        perror("IS ERROR IN SIZE = 0?");
        head -> head = NULL;
        head -> tail = NULL;
        perror("No SIZE = 0?");
    }
    else if (tmp == head -> head) {
        perror("IS ERROR IN HEAD");
        head -> head = tmp -> prev;
        head -> head -> next = NULL;
        perror("NO HEAD");
    }
    else if (tmp == head -> tail) {
        perror("IS ERROR IN tail");
        tmp -> next -> prev =NULL;
        head -> tail = tmp -> next;
        perror("no tail");
    }
    else {
        perror("IS ERROR IN regular?");
        tmp -> next -> prev = tmp -> prev;
        tmp -> prev -> next = tmp -> next;
        perror("NO REGULAR");
    }
    perror("ERROR NOT IN remove thread????????????????????????????");
    return tmp -> id;
}

int killThread(pthread_mutex_t * mutextounlock) {
    dead++;
    if (dead == threadNum) {
        pthread_cond_signal(&start);
    }
    if (mutextounlock != NULL) {
        pthread_mutex_unlock(mutextounlock);
    }
    exitcode = 1;
    pthread_exit(0);
}

void searchDir(char * path) {
    DIR * dir;
    DIR * dir_check;
    struct dirent *entry;
    struct stat st;
    char fullpath[PATH_MAX];

    // Already checked that this will work
    dir = opendir(path);

    errno = 0;
    while((entry = readdir(dir))){
        strcpy(fullpath,path);
        strcat(fullpath,"/");
        strcat(fullpath,entry->d_name);
        if (stat(fullpath,&st) <0){
            perror("Error with stat");
            closedir(dir);
            killThread(NULL);
        }
        if (S_ISDIR(st.st_mode) == 1) {
            if ((strcmp(entry -> d_name,".") != 0) && (strcmp(entry -> d_name,"..") != 0)) {
                dir_check = opendir(fullpath);
                if (dir_check == NULL) {
                    printf("Directory %s: Permission denied.\n", fullpath);
                    errno = 0;
                    continue;
                }
                closedir(dir_check);
                pthread_mutex_lock(&addRemoveMutex);
                add_directory_node(directoryList, fullpath);
                pthread_mutex_unlock(&addRemoveMutex);
            }
        }
        else {
            if (strstr(entry->d_name,searchTerm) != NULL) {
                printf("%s\n",fullpath);
                numFound++;
            }
        }
        errno =0;
    }
    if(errno != 0) {
        perror("An error has occurred with readdir");
        closedir(dir);
        killThread(NULL);
    }
    closedir(dir);
}

void * startDirCheck(void* num) {
    long my_id = (long) num;
    char path[PATH_MAX];
    int loc;
    pthread_mutex_lock(&startMutex);
    dontStart++;
    pthread_cond_wait(&start,&startMutex);
    dontStart--;
    pthread_mutex_unlock(&startMutex);
    while ((dead + sleeping != threadNum) || (!isDirQEmpty(directoryList)) || dontStart > 0) {
        pthread_mutex_lock(&addRemoveMutex);
        if (!(add_thread_node(threadSleepList,my_id))) {
            killThread(&addRemoveMutex);
        }
        pthread_mutex_unlock(&addRemoveMutex);
        pthread_mutex_lock(&mutex);
        if (isDirQEmpty(directoryList)) {
            sleeping++;
            fprintf(stderr,"thread is on way to sleep. there are %d threads sleeping and %d dead. dontstart=%d", threadSleepList->size,dead,dontStart);
            // Everyone is asleep and the directory folder is empty. Last thread starts exit process
            if ((dead + sleeping == threadNum) && (dontStart == 0)) {
                // Tell main to set other threads free
                perror("c");
                printf("MY id is %ld", my_id);
                loc = getThreadLocation(threadSleepList,my_id);
                perror("1");
                removeThreadNode(threadSleepList,loc);
                perror("2");
                pthread_mutex_unlock(&mutex);
                perror("3");
                pthread_cond_signal(&start);
                pthread_exit(0);
            }
            perror("B");
            pthread_cond_wait(&cvArray[my_id],&mutex);

            // Need to break to exit properly. Checks if main set the thread free
            if (dead + sleeping == threadNum && isDirQEmpty(directoryList)) {
                loc = getThreadLocation(threadSleepList,my_id);
                pthread_mutex_unlock(&mutex);
                pthread_exit(0);
            }
            sleeping--;
            pthread_mutex_unlock(&mutex);
            pthread_mutex_lock(&addRemoveMutex);
            loc = getThreadLocation(threadSleepList,my_id);
            removeThreadNode(threadSleepList,loc);
            strcpy(path, remove_directory_node(directoryList,loc));
            pthread_mutex_unlock(&addRemoveMutex);
            fprintf(stderr,"About to run searchdir from 1,thread:%d,dir:%d",threadSleepList ->size,directoryList ->size);
            searchDir(path);
        }
        else {
            //pthread_mutex_unlock(&mutex);
            perror("GOT HERE?");
            //pthread_mutex_lock(&addRemoveMutex);
            loc = getThreadLocation(threadSleepList,my_id);
            if (loc >= directoryList -> size){
                fprintf(stderr,"2:thread is on way to sleep. there are %d threads sleeping and %d dead. dontstart=%d", threadSleepList->size,dead,dontStart);
                sleeping++;
                if ((dead + sleeping == threadNum) && (dontStart == 0) && (isDirQEmpty(directoryList))) {
                    // Tell main to set other threads free
                    perror("c");
                    printf("MY id is %ld", my_id);
                    loc = getThreadLocation(threadSleepList,my_id);
                    perror("1");
                    removeThreadNode(threadSleepList,loc);
                    perror("2");
                    pthread_mutex_unlock(&mutex);
                    perror("3");
                    pthread_cond_signal(&start);
                    pthread_exit(0);
                }
                pthread_cond_wait(&cvArray[my_id],&mutex);
                loc = getThreadLocation(threadSleepList,my_id);
                if ((dead + sleeping == threadNum) && (isDirQEmpty(directoryList))) {
                    pthread_mutex_unlock(&mutex);
                    pthread_exit(0);
                }
                sleeping--;
            }
            pthread_mutex_unlock(&mutex);
            pthread_mutex_lock(&addRemoveMutex);
            removeThreadNode(threadSleepList,loc);
            strcpy(path, remove_directory_node(directoryList,loc));
            pthread_mutex_unlock(&addRemoveMutex);
            fprintf(stderr,"About to run searchdir from 2,thread:%d,dir:%d",threadSleepList ->size,directoryList ->size);
            searchDir(path);
        }
    }
    pthread_exit(0);
}

int main(int argc, char* argv[]) {
    long i;
    long j;
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&startMutex,NULL);
    pthread_mutex_init(&addRemoveMutex, NULL);
    pthread_mutex_init(&finishMutex,NULL);
    pthread_cond_init(&start,NULL);
    if (argc != 4) {
        errno = EINVAL;
        perror("There should be 3 paramaters");
        exit(1);
    }
    threadNum = atol(argv[3]);
    strcpy(searchTerm,argv[2]);
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
    while(dontStart != threadNum) {
        sleep(1);
    }

    pthread_cond_broadcast(&start);

    // Wait untill all threads are finished
    pthread_cond_wait(&start,&finishMutex);
    printf("Done searching, found %d files\n",numFound);
    perror("starting free");
    // All threads are finished, sets them free so they can exit. only main changes threadsleeplist.
    for (j = 0; j < sleeping -1; j++){
        fprintf(stderr,"IS ID WRONG?!?!?!?!?!?!,%lu\n",threadSleepList ->head->id);
        pthread_cond_signal(&cvArray[removeThreadNode(threadSleepList,0)]);
        perror("ID IS NOT WRONG!!!!!!!!!!");
        fprintf(stderr,"freeing %lu", threadSleepList ->head->id);
        fprintf(stderr,"left:%ld,threadleft:%d",(sleeping-1 -j),threadSleepList->size);
    }
    for (j = 0; j < threadNum; j++) {
        pthread_join(threads[i], NULL);
    }
    // Clean up and exit
    pthread_mutex_destroy(&startMutex);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&addRemoveMutex);
    pthread_mutex_destroy(&finishMutex);
    for (j =0; j<threadNum; j++) {
        pthread_cond_destroy(&cvArray[j]);
    }
    exit(exitcode);
}