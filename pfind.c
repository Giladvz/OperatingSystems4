#include <stdatomic.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

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

}threadNode;

typedef struct directoryNode {
    char * [PATH_MAX];
    directory_Node  * next;

} directoryNode;

static thead * create_sleep_list(void) {
    ThreadNode * head = (ThreadNode*)malloc(sizeof(thead));
    if(head == NULL) {
        exit(1);
    }
    head -> head = NULL;
    head -> tail = NULL;
    return head;
}

static thead * create_directory_list(void) {
    directoryNode * head = (directoryNode*)malloc(sizeof(dhead));
    if(head == NULL) {
        exit(1);
    }
    head -> head = NULL;
    head -> tail = NULL;
    return head;
}

