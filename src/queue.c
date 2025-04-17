#include "queue.h"


void init_queue(Queue *queue) {
    queue->top = -1;
}

void push(Queue *queue, uint8_t *tile) {
    if (queue->top < 3) {
        queue->top++;
        for (int i = 0; i < TILE_SIZE; i++) {
            queue->array[queue->top][i] = tile[i]; 
        }
    }
}

int get_top(Queue *queue) {
    return queue->top;
} 

uint8_t* pop(Queue *queue) {
    uint8_t *return_val;
    if (queue->top >= 0) {
        return_val = queue->array[0];
    }

    for (int i = 0; i < TILE_SIZE; i++) {
        queue->array[0][i] = queue->array[1][i];
        queue->array[1][i] = queue->array[2][i];
        queue->array[2][i] = 0;
    }

    return return_val;
}