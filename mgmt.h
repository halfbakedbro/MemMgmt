#ifndef __MGMT_H__
#define __MGMT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>


// To Use stack trace project needs to compile with -rdynamic flag
#ifdef MGMT_STACK_TRACE
#include <execinfo.h>
#ifndef MGMT_STACK_TRACE_MAX
#define MGMT_STACK_TRACE_MAX    30
#endif
#endif  // MGMT_STACK_TRACE

#define init()              _qinit()
#define cleanup()           _qcleanup()
#define qmalloc(sz)         _qalloc(sz, 0, __func__, __FILE__, __LINE__)
#define qcalloc(num,sz)     _qalloc((num) * (sz), 1, __func__, __FILE__, __LINE__)
#define qrealloc(ptr,sz)    _qrealloc(ptr, sz, __func__, __FILE__, __LINE__)
#define qfree(ptr)          _qfree(ptr, __func__, __FILE__, __LINE__)
#define qsize(ptr)          _qsize(ptr, __func__, __FILE__, __LINE__)

void qmalloc_set_oom_handler(void (*oom_handler)(size_t));

void _qinit();
void _qcleanup();

void *_qalloc(size_t, int, const char*, const char*, unsigned);
void *_qrealloc(void*, size_t, const char*, const char*, unsigned);
void *_qfree(void*, const char*, const char*, unsigned);
void *_qsize(void*, const char*, const char*, unsigned);

void qdump(FILE*);


size_t total_usage(void*);

int it_has(void *);

typedef struct q_node_t{
    struct q_node_t *prev, *next;
    const char *file;
    const char *func;
    size_t line;
    size_t sz;
#ifdef MGMT_STACK_TRACE
    void *stacktrace[MGMT_STACK_TRACE_MAX];
    size_t stacktrace_sz;
#endif
}q_node, *pq_node;


#endif // __MGMT_H__