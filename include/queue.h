#include <stdint.h>

#define STACK_SIZE 3
#define TILE_SIZE 8

typedef struct Queue
{
    int top;
    uint8_t array[STACK_SIZE][TILE_SIZE];
} Queue;

void init_queue(Queue *queue);

void push(Queue *queue, uint8_t *tile);
int get_top(Queue *queue);
uint8_t* pop(Queue *queue);



