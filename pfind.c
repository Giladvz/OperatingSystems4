#include <stdatomic.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>

typdef struct threadHead {
    threadNode * head;
    threadNode * tail;
}thead;

typdef struct directoryHead {
    directoryNode * head;
    directoryNode * tail;
}dhead;

typedef struct threadNode {
    int id;
    threadNode  * next;
    threadNode  * prev;

}threadNode;

typedef struct directoryNode {
    char * path[PATH_MAX];
    directory_Node * next;
    directory_Node * prev;

} directoryNode;

thead * create_sleep_list(void) {
    thead * head = (ThreadNode*)malloc(sizeof(thead));
    if(head == NULL) {
        printf("Error with malloc");
        exit(1);
    }
    head -> head = NULL;
    head -> tail = NULL;
    return head;
}

hhead * create_directory_list(void) {
    dhead * head = (directoryNode*)malloc(sizeof(dhead));
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
        return NULL;
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
    new_node -> path = fullpath;
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

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("There should be 3 paramaters");
    }
    DIR * check_dir = opendir(argv[1]);
    if (check_dir == NULL) {
        printf("Directory %s: Permission denied.\n",argv[1]);
        exit(1);
    }
    if(closedir(check_dir) <0 ) {
        printf("Error closing dir");
        exit(1);
    }
    pthread_t * threadlist = (pthread_t *)calloc(atoi(argv[3]),sizeof(pthread_t));



    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    // Clean up and exit

}