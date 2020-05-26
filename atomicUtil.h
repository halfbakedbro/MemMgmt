#include <pthread.h>

#ifndef __ATOMICUTIL_H__
#define __ATOMICUTIL_H__


#define atomicAlloc(var, node) do {\
    pthread_mutex_lock(&var ## _mutex); \
    if (q_head) { \
        q_head->prev = node;\
        node->next = q_head; \
    } \
    q_head = node;\
    pthread_mutex_unlock(&var ## _mutex);\
}while(0)

#define atomicFree(var, node) do { \
    pthread_mutex_lock(&var ## _mutex); \
    if (node == q_head) q_head = node->next; \
    if (node->prev) node->prev->next = node->next; \
    if (node->next) node->next->prev = node->prev; \
    pthread_mutex_unlock(&var ## _mutex); \
}while (0)

#define atomicHasNode(var, node, dummy, has) do {\
    pthread_mutex_lock(&var ## _mutex); \
    dummy = _q_head; \
    while (dummy != NULL) { \
        if (node == dummy) {    \
            has = 1;    \
            break;  \
        } \
        dummy = dummy->next; \
    } \

    pthread_mutex_unlock(&var ## _mutex); \
}while (0)




#endif  // __ATOMICUTIL_H__