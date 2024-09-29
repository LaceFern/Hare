#ifndef QUEUE_INCLUDED
#define QUEUE_INCLUDED
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "./include/lock_queue.h"

typedef struct QueueNode {
    uint8_t     mode;
    uint16_t    client_id;
    uint16_t    srcPort;
    uint32_t    txnID;
    uint32_t    srcIP;
    struct QueueNode *next;
} queue_node;

typedef struct QueueList {
    int size_of_queue;
    queue_node *head;
    queue_node *tail;
} queue_list;

static void queue_init(queue_list *q) {
    q->size_of_queue = 0;
    q->head = NULL;
    q->tail = NULL;
}

static int enqueue(queue_list *q, uint8_t mode, uint16_t client_id, uint16_t srcPort, uint32_t txnID, uint32_t srcIP) {
    queue_node *new_node = (queue_node *) malloc(sizeof(queue_node));
    if (new_node == NULL) {
        fprintf(stderr, "MALLOC ERROR.\n");
        return -1;
    }

    new_node->mode = mode;
    new_node->client_id = client_id;
    new_node->srcPort = srcPort;
    new_node->txnID = txnID;
    new_node->srcIP = srcIP;
    new_node->next = NULL;

    if (q->size_of_queue == 0) {
        q->head = new_node;
        q->tail = new_node;
    }
    else {
        q->tail->next = new_node;
        q->tail = new_node;
    }
    q->size_of_queue ++;
    return 0;
}

static void dequeue(queue_list *q, uint8_t* mode, uint16_t* client_id, uint16_t* srcPort, uint32_t* txnID, uint32_t* srcIP) {
    if ((q != NULL) && (q->size_of_queue > 0)) {
        queue_node *temp = q->head;
        (*mode) = temp->mode;
        (*client_id) = temp->client_id;
        (*srcPort) = temp->srcPort;
        (*txnID) = temp->txnID;
        (*srcIP) = temp->srcIP;
        free(temp);
        if (q->size_of_queue > 1) {
            q->head = q->head->next;
        }
        else {
            q->head = NULL;
            q->tail = NULL;
        }

        q->size_of_queue --;
        
    }
    return;
}

static void queue_clear(queue_list *q) {
    queue_node *temp;
    while (q->size_of_queue > 0) {
        temp = q->head;
        q->head = temp->next;
        free(temp);
        q->size_of_queue --;
    }

    q->head = NULL;
    q->tail = NULL;
    return;
}

static int get_queue_size(queue_list *q) {
    if (q == NULL)
        return 0;
    return q->size_of_queue;
}

#endif

