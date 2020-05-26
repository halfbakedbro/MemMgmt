/**
 * 
 * Need To maintan policies to handle the OOM (out of Memeory)
 * Condition. There is no songle right approach.
 * 
 * Now there are Three major policies for handling OOM;
 * 1. Recovery
 *  1.1. It will be most difficult to implement
 *  1.2. Gracefully recover
 *      1.2.1. Release some resource and try again
 *      1.2.2. Save the users work and exit
 *      1.2.3. Clean up temporary resources and exit
 * 
 * 2. Abort
 *  2.1. This policy is simple and familiar: Whenno memory is available, print a polite
 *       error message and exit
 * 3. SegFault
 *  3.1. The segfault policy is the most simplistic of all
 */

#include "mgmt.h"
#include "atomicUtil.h"


#define update_qmalloc_stat(__d) do { \
    atomicAlloc(stat, __d);\
}while (0)

#define update_qmalloc_free(__d) do { \
    atomicFree(stat, __d); \
}while (0)

#define qmalloc_has_node(__d, __t) do { \
    pq_node __n; \
    atomicHasNode(stat, __d, __n, __t); \
}while (0)

static pq_node q_head;
static int isInit = 0;
pthread_mutex_t stat_mutex;

static void qmalloc_default_oom(size_t size) {
    fprintf(stderr, "qmalloc: Out of memory trying to allocate %zu bytes\n", size);

    fflush(stderr);
}

static void (*oom_handler)(size_t size) = qmalloc_default_oom;

int _q_has_node(pq_node arg) {
    pq_node node = q_head;

    while (node != NULL) {
        if (node == arg) return 1;
        node = node->next;
    }

    return 0;
}

void _q_abrt(void) {
#ifdef MGMT_STACK_TRACE
    void *array[MGMT_STACK_TRACE_MAX];
    size_t sz = backtrace(array, MGMT_STACK_TRACE_MAX);
    backtrace_symbols_fd(array, MGMT_STACK_TRACE_MAX, fileno(stderr))
#endif

    abort();
}

#ifdef MGMT_HANDLE_SEGFAULT
void segfault_segaction(int signal, siginfo_t *si, void *arg) {

    fprintf(stderr, "caught segfault at address %p\n", si->si_addr);
    exit(0);
}
#endif

void _qinit() {
    if ( 0 != pthread_mutex_init(&stat_mutex, NULL) {
        fprintf(stderr, "Mutex init failed\n");
        return;
    }
#ifdef MGMT_HANDLE_SEGFAULT
    struct sigaction sa;

    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segfault_segaction;
    sa.sa_flags = SA_SIGINFO;

    sigaction(SIGSEGV, &sa, NULL);
#endif

    isInit = 1;

    /********* some other stuff **********/
}

void _qcleanup() {
    if (isInit) {
        pthread_mutex_destroy(&stat_mutex);
    }
}

/****************************************************************
 * 
 * _QALLOC : Allocates the memory based on input value
 * 
 *  INPUT PARAMETERS:
 *                      size_t sz : Size to be allocated in bytes
 *                      int zeroset: Called for malloc or Calloc
 *                      const char *func: Name of a function from which it is called
 *                      const char *file: File name of the calling function
 *                      unsigned line : Line number from where it is called
 * 
 *  OUTPUT PARAMETERS:
 *                      pointer to the allocated memory
 * ********************************************************************/

void *_qalloc(size_t sz, int zeroset, const char *func, const char *file, unsigned line) {
    pq_node node = NULL;

    if (zeroset) {
        node = calloc(sizeof(*node) + sz, 1);
    }else {
        node = malloc(sizeof(*node) + sz);
    }

    /* Unable to allocate memory. Proper logging where it is failed */

    if (node == NULL) {
#ifdef MGMT_HANDLE_ABRT
        fprintf(stderr, "Couldn't allocate : %s, line %u\n", file, line);
        _q_abrt();
#else
        //oom_handler(sz);
        return NULL;
#endif
    }

    node->sz = sz;
    node->line = line;
    node->func = func;
    node->file = file;

#ifdef MGMT_STACK_TRACE
    node->stacktrace_sz = backtrace(node->stacktrace, MGMT_STACK_TRACE_MAX);
#endif

    update_qalloc_stat(node);

    return (char*)node + sizeof(*node);
}

/*************************************************************
 *  _QFREE : Frees the memory allocated
 * 
 * INPUT PARAMETERS : 
 *                      void *ptr : Pointer to the memory
 *                      const char *func : Name of the function from where it is called
 *                      const char *file : Filename for where it is called
 *                      unsigned line : Line number
 * 
 * OUTPUT PARAMETERS : 
 *                      VOID
 * **********************************************************************/

void _qfree(void *ptr, const char* func, const char* file, unsigned line) {
    
    if (ptr == NULL) return

    int itHas = 0;

    q_node *node = (q_node*)((char*)ptr - sizeof(*node));

    qmalloc_has_node(node, itHas);

    if (!itHas) {
        fprintf(stderr, "Couldn't free : %p %s , line %u\n", ptr, file , line);
        return;
    }

    update_qalloc_free(node);

    free(node);
}

void *_realloc(void *ptr, size_t sz, const char *func, const char *file, unsigned line) {

    if (ptr == NULL) return _qalloc(sz, 0 , func, file, line);

    pq_node node = (q_node*)((char*)ptr - sizeof(*node));
    pq_node old_node = node;

    if (!_q_has_node(node)){
        fprintf(stderr, "Realloc called wrong: %p %s %s, line %u\n", ptr, func, file, line);
        return NULL;
    }

    node = realloc(node, sizeof(*node) + sz);

    if (node == NULL) {
#ifdef MGMT_HANDLE_ABRT
        fprintf(stderr, "Couldn't realloc: %s %s, line %u\n", func, file, line);
        _q_abrt();
#else
        oom_handler(sz);
        return NULL;
#endif
    }

    node->sz = sz;
    node->file = file;
    node->func = func;
    node->line = line;

    if(q_head == old_node) q_head = node;
    if (node->prev) node->prev->next = node;
    if (node->next) node->next->prev = node;

    return (char*)node + sizeof(*node);
}


