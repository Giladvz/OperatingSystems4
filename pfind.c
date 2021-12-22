#include <stdatomic.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typdef struct thread_head {
    thread_Node * head;
    thread_Node * tail;
}thead;

typdef struct directory_head {
     * head;
    thread_Node * tail;
}dhead;

typedef struct thread_Node {
    int thread_id;
    thread_Node  * next;

}thread_Node;

typedef struct directory_Node {
    char * [PATH_MAX];
    directory_Node  * next;

} directory_Node;

