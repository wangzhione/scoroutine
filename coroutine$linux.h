#if !defined(_H_COROUTINE$WINDS) && defined(__GNUC__)
#define _H_COROUTINE$WINDS

#include "coroutine.h"
#include <ucontext.h>
#include <stddef.h>
#include <stdint.h>

// co - 声明协程结构 和 cg - 协程管理器结构
struct co {
    co_f func;              // 协程体执行
    void * arg;             // 用户输入的参数
    int status;             // 当前协程运行状态 CO_*
    ucontext_t ctx;         // 当前协程运行的上下文环境

    char * stack;           // 当前协程栈指针
    int cap;                // 当前栈的容量
    int size;               // 当前栈的大小
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
inline void co_savestack(struct co * c, char * top) {
    // x86 CPU 栈从高位向低位增长
    char dummy = 0;
    ptrdiff_t size = top - &dummy;
    assert(size <= INT_STACK);
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
    int cnt;                // 当前存在的协程个数

    int running;            // 当前协程中运行的协程 id
    ucontext_t ctx;         // 当前协程上下文对象
    char stack[INT_STACK];  // 当前协程中用的栈
};

// co_run - 协程运行的主体
static void co_run(uint32_t l32, uint32_t h32) {
    uintptr_t ptr = (uintptr_t)l32 | ((uintptr_t)h32 << 32);
    struct comng * cg = (struct comng *)ptr;
    int id = cg->running;
    struct co * c = cg->cs[id];

    // 执行协程体
    c->func(cg, c->arg);
    c->status = CO_DEAD;

    co_die(c);
    cg->cs[id] = NULL;
    --cg->cnt;
    cg->idx = id;
    cg->running = -1;
}

//
// co_open - 开启协程系统, 并返回创建的协程管理对象
// return  : 创建的协程管理对象
//
comng_t 
co_open(void) {
    struct comng * cg = malloc(sizeof(struct comng));
    assert(NULL != cg);
    cg->running = -1;
    cg->cs = calloc(INT_COROUTINE, sizeof(struct co *));
    assert(NULL != cg->cs);
    cg->cap = INT_COROUTINE;
    cg->idx = cg->cnt = 0;
    return cg;
}

//
// co_close - 关闭已经开启的协程系统
// cg      : 协程管理对象
// return   : void
//
void 
co_close(comng_t cg) {
    int i = 0;
    while (i < cg->cap) {
        struct co * c = cg->cs[i];
        if (c) {
            co_die(c);
            cg->cs[i] = NULL;
        }
        ++i;
    }

    free(cg->cs);
    cg->cs = NULL;
    free(cg);
}

//
// co_create - 创建一个协程, 此刻是就绪态
// cg        : 协程管理对象
// func      : 协程体执行的函数体
// arg       : 协程体中传入的参数
// return    : 返回创建好的协程 id
//
int 
co_create(comng_t cg, co_f func, void * arg) {
    int cap = cg->cap;
    struct co ** cs = cg->cs;
    struct co * c = co_new(func, arg);

    // 下面开始寻找, 如果数据足够的话
    if (cg->cnt < cg->cap) {
        // 当循环队列去查找
        int idx = cg->idx;
        do {
            if (NULL == cs[idx]) {
                cs[idx] = c;
                ++cg->cnt;
                ++cg->idx;
                return idx;
            }
            idx = (idx + 1) % cap;
        } while (idx != cg->idx);

        assert(idx == cg->idx);
        return -1;
    }

    // 这里需要重新构建空间
    cs = realloc(cs, sizeof(struct co *) * cap<<1);
    assert(NULL != cs);
    memset(cs + cap, 0, sizeof(struct co *) * cap);
    cs[cg->idx] = c;
    cg->cap = cap<<1;
    cg->cs = cs;
    ++cg->cnt;
    return cg->idx++;
}

//
// co_resume - 通过协程 id 激活协程
// cg        : 协程管理对象
// id        : 协程id
// return    : void
//
void 
co_resume(comng_t cg, int id) {
    uintptr_t ptr;
    struct co * c;
    int status, running = cg->running;
    assert(running == -1 && id >= 0 && id < cg->cap);

    // 下面是协程 CO_READY 和 CO_SUSPEND 处理
    c = cg->cs[id];
    if ((!c) || (status = c->status) == CO_DEAD)
        return;

    cg->running = id;
    c->status = CO_RUNNING;
    switch (status) {
    case CO_READY:
        // 兼容 x64 指针通过 makecontext 传入
        ptr = (uintptr_t)cg;
        // 构建栈和运行链
        getcontext(&c->ctx);
        c->ctx.uc_link = &cg->ctx;
        c->ctx.uc_stack.ss_sp = cg->stack;
        c->ctx.uc_stack.ss_size = INT_STACK;
        makecontext(&c->ctx, (void(*)())co_run, 2, (uint32_t)ptr, (uint32_t)(ptr >> 32));
        // 保存当前运行状态到 cg->ctx, 然后跳转到 co->ctx 运行环境中
        swapcontext(&cg->ctx, &c->ctx);
        break;
    case CO_SUSPEND:
        // stack add is high -> low
        memcpy(cg->stack + INT_STACK - c->size, c->stack, c->size);
        swapcontext(&cg->ctx, &c->ctx);
        break;
    default:
        assert(c->status && 0);
    }
}

//
// co_yield - 关闭当前正在运行的协程, 协程暂停
// cg       : 协程管理对象
// return   : void
//
void 
co_yield(comng_t cg) {
    struct co * c;
    int id = cg->running;
    if ((id < 0) || (id >= cg->cap) || !(c = cg->cs[id]))
        return;

    assert((char *)&c > cg->stack);
    co_savestack(c, cg->stack + INT_STACK);

    c->status = CO_SUSPEND;
    cg->running = -1;

    swapcontext(&c->ctx, &cg->ctx);
}

//
// co_running - 当前协程系统中运行的协程 id
// cg         : 协程系统管理器
// retrunr    : 返回 < 0 表示没有协程在运行
//
inline int 
co_running(comng_t cg) {
    return cg->running;
}

//
// co_status - 得到当前协程状态
// cg        : 协程管理对象
// id        : 协程 id
// return    : 返回对应协程状态信息
//
inline int co_status(comng_t cg, int id) {
    assert(cg && id >= 0 && id < cg->cap);
    return cg->cs[id] ? cg->cs[id]->status : CO_DEAD;
}

#endif//_H_COROUTINE$WINDS
