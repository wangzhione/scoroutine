#if !defined(COROUTINE$LINE_H) && !defined(_WIN32)
#define COROUTINE$LINE_H

#include "coroutine.h"

#if defined(__linux__)
#  include <ucontext.h>
#else
#  include <sys/ucontext.h>
#endif

// 声明 co - 协程结构 和 comng - 协程管理器结构
struct co {
    co_f func;              // 协程执行行为
    void * arg;             // 协程执行参数
    int status;             // 当前协程状态 CO_*
    ucontext_t ctx;         // 当前协程运行的上下文环境

    char * stack;           // 当前协程栈指针
    ptrdiff_t cap;          // 当前栈的容量
    ptrdiff_t size;         // 当前栈使用大小
};

// co_new - 构建 struct co 协程对象
inline struct co * co_new(co_f func, void * arg) {
    struct co * c = malloc(sizeof(struct co));
    assert(c && func);
    c->func = func;
    c->arg = arg;
    c->status = CO_READY;

    c->stack = NULL;
    c->cap = c->size = 0;
    return c;
}

// co_die - 销毁一个协程对象
inline void co_die(struct co * c) {
    free(c->stack);
    free(c);
}

// co_savestack 保存当前运行的栈信息
static inline void co_savestack(struct co * c, char * top) {
    // x86 CPU 栈从高位向低位增长
    char dummy = 0;
    ptrdiff_t size = top - &dummy;
    assert(size <= STACK_INT);
    if (c->cap < size) {
        free(c->stack);
        c->stack = malloc(c->cap = size);
        assert(c->stack);
    }
    c->size = size;
    memcpy(c->stack, &dummy, size);
}

struct comng {
    struct co ** cs;        // 协程对象集, 循环队列
    int cap;                // 协程对象集容量
    int idx;                // 当前协程集中轮询到的索引
    int len;                // 当前协程集存在的协程个数

    int running;            // 当前协程集中运行的协程 id
    ucontext_t ctx;         // 当前协程集上下文对象
    char stack[STACK_INT];  // 当前协程集中用的栈
};

// co_run - 协程运行的主体
static void co_run(uint32_t l32, uint32_t h32) {
    uintptr_t ptr = (uintptr_t)l32 | ((uintptr_t)h32 << 32);
    struct comng * g = (struct comng *)ptr;
    int id = g->running;
    struct co * c = g->cs[id];

    // 执行协程体
    c->func(g, c->arg);

    c->status = CO_DEAD;
    co_die(c);

    g->cs[id] = NULL;
    --g->len;
    g->idx = id;
    g->running = -1;
}

//
// co_open - 开启协程系统, 返回协程管理器对象
// return   : 创建的协程管理器对象
//
comng_t 
co_open(void) {
    struct comng * g = malloc(sizeof(struct comng));
    assert(NULL != g);
    g->running = -1;
    g->cs = calloc(COROUTINE_INT, sizeof(struct co *));
    assert(NULL != g->cs);
    g->cap = COROUTINE_INT;
    g->idx = g->len = 0;
    return g;
}

//
// co_close - 关闭开启的协程系统
// g        : 协程管理器对象
// return   : void
//
void 
co_close(comng_t g) {
    for (int i = 0; i < g->cap; ++i) {
        struct co * c = g->cs[i];
        if (c) {
            co_die(c);
            g->cs[i] = NULL;
        }
    }

    free(g->cs);
    g->cs = NULL;
    free(g);
}

//
// co_create - 创建一个就绪态协程
// g        : 协程管理器对象
// func     : 协程体执行的函数体
// arg      : 协程体中传入的参数
// return   : 返回协程 id
//
int 
co_create(comng_t g, co_f func, void * arg) {
    int cap = g->cap;
    struct co ** cs = g->cs;
    struct co * c = co_new(func, arg);

    // 下面开始寻找, 如果数据足够的话
    if (g->len < g->cap) {
        // 当循环队列去查找
        int idx = g->idx;
        do {
            if (NULL == cs[idx]) {
                cs[idx] = c;
                ++g->len;
                ++g->idx;
                return idx;
            }
            idx = (idx + 1) % cap;
        } while (idx != g->idx);

        assert(idx == g->idx);
        return -1;
    }

    // 这里需要重新构建空间
    cs = realloc(cs, sizeof(struct co *) * cap<<1);
    assert(NULL != cs);
    memset(cs + cap, 0, sizeof(struct co *) * cap);
    cs[g->idx] = c;
    g->cap = cap<<1;
    g->cs = cs;
    ++g->len;
    return g->idx++;
}

//
// co_resume - 通过协程 id 激活协程
// g        : 协程管理器对象
// id       : 协程id
// return   : void
//
void 
co_resume(comng_t g, int id) {
    int status, running = g->running;
    assert(running == -1 && id >= 0 && id < g->cap);

    // 下面是协程 CO_READY 和 CO_SUSPEND 处理
    struct co * c = g->cs[id];
    if ((!c) || (status = c->status) == CO_DEAD)
        return;

    g->running = id;
    c->status = CO_RUNNING;
    switch (status) {
    case CO_READY: {
        // 兼容 x64 指针通过2个 4 字节传入到 makecontext 中
        uintptr_t ptr = (uintptr_t)g;
        uint32_t l32 = (uint32_t)ptr;
        uint32_t h32 = (uint32_t)(ptr >> 32);

        // 构建栈和上下文环境运行链
        getcontext(&c->ctx);
        c->ctx.uc_link = &g->ctx;
        c->ctx.uc_stack.ss_sp = g->stack;
        c->ctx.uc_stack.ss_size = STACK_INT;
        makecontext(&c->ctx, (void(*)())co_run, 2, l32, h32);
        // 保存当前运行状态到 g->ctx, 然后跳转到 co->ctx 运行环境中
        swapcontext(&g->ctx, &c->ctx);
    }
    break;
    case CO_SUSPEND:
        // stack add is high -> low
        memcpy(g->stack + STACK_INT - c->size, c->stack, c->size);
        swapcontext(&g->ctx, &c->ctx);
    break;
    default:
        assert(c->status && 0);
    }
}

//
// co_yield - 暂停正在运行的协程
// g        : 协程管理器对象
// return   : void
//
void 
co_yield(comng_t g) {
    int id = g->running;
    if (id < 0 || id >= g->cap || !g->cs[id])
        return;

    struct co * c = g->cs[id];
    assert((char *)&c > g->stack);
    co_savestack(c, g->stack + STACK_INT);

    c->status = CO_SUSPEND;
    g->running = -1;

    swapcontext(&c->ctx, &g->ctx);
}

//
// co_running - 获取正在运行的协程 id
// g        : 协程系统管理器
// retrunr  : < 0 表示没有协程在运行
//
inline int 
co_running(comng_t g) {
    return g->running;
}

//
// co_status - 获取对应协程的状态
// g        : 协程管理器对象
// id       : 协程 id
// return   : 协程状态
//
inline int co_status(comng_t g, int id) {
    assert(g && id >= 0 && id < g->cap);
    return g->cs[id] ? g->cs[id]->status : CO_DEAD;
}

#endif//COROUTINE$LINE_H
