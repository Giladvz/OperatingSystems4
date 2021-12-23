#include <stdatomic.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>

pthread_mutex_t mutex;
pthread_cond_t isHeadCV;
pthread_cond_t isNotHeadCV;

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



thead * create_sleep_list(void) {
    thead * head = (thead*)malloc(sizeof(thead));
    if(head == NULL) {
        printf("Error with malloc");
        exit(1);
    }
    head -> head = NULL;
    head -> tail = NULL;
    return head;
}

dhead * create_directory_list(void) {
    dhead * head = (dhead*)malloc(sizeof(dhead));
    if(head == NULL) {
        printf("Error with malloc");
        exit(1);
    }
    head -> head = NULL;
    head -> tail = NULL;
    return head;
}

void add_directory_node(dhead * head, char * fullPath) {
    directoryNode * new_node = (directoryNode *)malloc(sizeof(directoryNode));
    if (new_node == NULL) {
        printf("Error with malloc");
        return;
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

void * startDirCheck(void* num){
    long my_id = (long)num;
    printf("Starting thread number %lu",my_id);
    pthread_cond_wait(isHeadCV);
    return 0;
}

int main(int argc, char* argv[]) {
    long i;
    dhead directoryList;
    thead threadSleepList;
    int isEveryOneAsleep;


    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&isHeadCV, NULL);
    pthread_cond_init(&isNotHeadCV, NULL);

    if (argc != 4) {
        printf("There should be 3 paramaters\n");
        exit(1);
    }
    int threadNum = atoi(argv[3]);
    DIR * check_dir = opendir(argv[1]);
    if (check_dir == NULL) {
        printf("Directory %s: Permission denied.\n",argv[1]);
        exit(1);
    }
    if(closedir(check_dir) <0 ) {
        printf("Error closing dir");
        exit(1);
    }

    // Creating queues
    directoryList = create_directory_list();
    threadSleepList = create_sleep_list();
    // Adding root directory to queue
    add_directory_node(directoryList,argv[1]);

    pthread_t * threads = (pthread_t *)calloc(threadNum,sizeof(pthread_t));
    for (i=0; i<threadNum;i++){
        if ((pthread_create(&threads[i], NULL, startDirCheck, (void *)i)) < 0) {
            printf("Error creating threads");
            exit(1);
        }
    }
    sleep 1;


    for (int i = 0; i < threadNum; i++) {
        pthread_join(threads[i], NULL);
    }
    // Clean up and exit
    pthread_mutex_destroy(mutex);
    pthread_cond_destroy(isHeadCV);
    pthread_cond_destroy(isNotHeadCV);
}