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
    threadNode  * prev;

}threadNode;

typedef struct directoryNode {
    char * path[PATH_MAX];
    directory_Node * next;
    directory_Node * prev;

} directoryNode;

thead * create_sleep_list(void) {
    ThreadNode * head = (ThreadNode*)malloc(sizeof(thead));
    if(head == NULL) {
        exit(1);
    }
    head -> head = NULL;
    head -> tail = NULL;
    return head;
}

hhead * create_directory_list(void) {
    directoryNode * head = (directoryNode*)malloc(sizeof(dhead));
    if(head == NULL) {
        exit(1);
    }
    head -> head = NULL;
    head -> tail = NULL;
    return head;
}

void add_directory_node(dhead * head, char * fullPath) {
    directoryNode * new_node = (directoryNode *)malloc(sizeof(directoryNode));
    if (new_node == NULL) {
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
