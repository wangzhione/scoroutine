#if !defined(_H_SCOROUTINE$LINUX) && defined(__GNUC__)
#define _H_SCOROUTINE$LINUX

#include "scoroutine.h"

#include <ucontext.h>
#include <stddef.h>
#include <stdint.h>

// 声明协程结构 和 协程管理器结构
struct sco {
    char * stack;           // 当前协程栈指针
    ptrdiff_t cap;          // 当前栈的容量
    ptrdiff_t cnt;          // 当前栈的大小

    ucontext_t ctx;         // 当前协程运行的上下文环境
    sco_f func;             // 协程体执行
    void * arg;             // 用户输入的参数
    int status;             // 当前协程运行状态 SCO_*
};

struct scomng {
    char stack[INT_STACK];  // 当前协程中开辟的栈对象

    ucontext_t main;        // 当前协程上下文对象
    int running;            // 当前协程中运行的协程id

    struct sco ** cos;      // 协程对象集, 循环队列
    int cap;                // 协程对象集容量
    int idx;                // 当前协程集中轮询到的索引
    int cnt;                // 当前存在的协程个数
};

//
// sco_open - 开启协程系统函数, 并返回创建的协程管理器
// return   : 返回创建的协程对象
//
inline scomng_t
sco_open(void) {
    struct scomng * comng = malloc(sizeof(struct scomng));
    assert(NULL != comng);
    comng->running = -1;
    comng->cos = calloc(INT_COROUTINE, sizeof(struct sco *));
    assert(NULL != comng->cos);
    comng->cap = INT_COROUTINE;
    comng->idx = comng->cnt = 0;
    return comng;
}

// 销毁一个协程对象
inline void sco_delete(struct sco * co) {
    free(co->stack);
    free(co);
}

//
// sco_open - 开启协程系统函数, 并返回创建的协程管理器
// return   : 返回创建的协程对象
//
void
sco_close(scomng_t sco) {
    int i = 0;
    while (i < sco->cap) {
        struct sco * co = sco->cos[i];
        if (co) {
            sco_delete(co);
            sco->cos[i] = NULL;
        }
        ++i;
    }

    free(sco->cos);
    sco->cos = NULL;
    free(sco);
}

// 构建 struct sco 协程对象
inline struct sco * sco_new(sco_f func, void * arg) {
    struct sco * co = malloc(sizeof(struct sco));
    assert(co && func);
    co->func = func;
    co->arg = arg;
    co->status = SCO_READY;

    co->stack = NULL;
    co->cap = co->cnt = 0;

    return co;
}

//
// sco_create - 创建一个协程, 此刻是就绪态
// sco      : 协程管理器
// func     : 协程体执行的函数体
// arg      : 协程体中传入的参数
// return   : 返回创建好的协程id
//
int
sco_create(scomng_t sco, sco_f func, void * arg) {
    int cap = sco->cap;
    struct sco ** cos = sco->cos;
    struct sco * co = sco_new(func, arg);

    // 下面开始寻找, 如果数据足够的话
    if (sco->cnt < sco->cap) {
    // 当循环队列去查找
    int idx = sco->idx;
    do {
        if (NULL == cos[idx]) {
            cos[idx] = co;
            ++sco->cnt;
            ++sco->idx;
            return idx;
        }
        idx = (idx + 1) % cap;
    } while (idx != sco->idx);

    assert(idx == sco->idx);
    return -1;
    }

    // 这里需要重新构建空间
    cos = realloc(cos, sizeof(struct sco *) * cap<<1);
    assert(NULL != cos);
    memset(cos + cap, 0, sizeof(struct sco *) * cap);
    cos[sco->idx] = co;
    sco->cap = cap<<1;
    sco->cos = cos;
    ++sco->cnt;
    return sco->idx++;
}

// 协程运行的主体
static void _sco_main(uint32_t low32, uint32_t hig32) {
    uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hig32 << 32);
    struct scomng * comng = (struct scomng *)ptr;
    int id = comng->running;
    struct sco * co = comng->cos[id];

    // 执行协程体
    co->func(comng, co->arg);
    co = comng->cos[id];
    co->status = SCO_DEAD;

    sco_delete(co);
    comng->cos[id] = NULL;
    comng->running = -1;
    comng->idx = id;
    --comng->cnt;
}

//
// sco_resume - 通过协程id激活协程
// sco      : 协程系统管理器
// id       : 具体协程id, sco_create 返回的协程id
// return   : void
//
void
sco_resume(scomng_t sco, int id) {
    uintptr_t ptr;
    struct sco * co;
    int status, running = sco->running;
    assert(running == -1 && id >= 0 && id < sco->cap);

    // 下面是协程 SCO_READY 和 SCO_SUSPEND 处理
    co = sco->cos[id];
    if ((!co) || (status = co->status) == SCO_DEAD)
        return;

    sco->running = id;
    co->status = SCO_RUNNING;
    switch (status) {
    case SCO_READY:
        // 兼容 x64指针通过 makecontext 传入
        ptr = (uintptr_t)sco;
        // 构建栈和运行链
        getcontext(&co->ctx);
        co->ctx.uc_link = &sco->main;
        co->ctx.uc_stack.ss_sp = sco->stack;
        co->ctx.uc_stack.ss_size = INT_STACK;
        makecontext(&co->ctx, (void(*)())_sco_main, 2, (uint32_t)ptr, (uint32_t)(ptr >> 32));
        // 保存当前运行状态到 sco->main, 然后跳转到 co->ctx 运行环境中
        swapcontext(&sco->main, &co->ctx);
        break;
    case SCO_SUSPEND:
        // stack add is high -> low
        memcpy(sco->stack + INT_STACK - co->cnt, co->stack, co->cnt);
        swapcontext(&sco->main, &co->ctx);
        break;
    default:
        assert(co->status && 0);
    }
}

// 保存当前运行的堆栈信息
inline void sco_savestack(struct sco * co, char * top) {
    char dummy = 0;
    ptrdiff_t size = top - &dummy;
    assert(size <= INT_STACK);
    if (co->cap < size) {
        co->cap = size;
        free(co->stack);
        co->stack = malloc(size);
    }
    co->cnt = size;
    memcpy(co->stack, &dummy, size);
}

//
// sco_yield - 关闭当前正在运行的协程, 让协程处理暂停状态
// sco      : 协程系统管理器
// return   : void
//
void
sco_yield(scomng_t sco) {
    struct sco * co;
    int id = sco->running;
    if ((id < 0 || id >= sco->cap) || !(co = sco->cos[id]))
        return;

    assert((char *)&co > sco->stack);
    sco_savestack(co, sco->stack + INT_STACK);

    co->status = SCO_SUSPEND;
    sco->running = -1;

    swapcontext(&co->ctx, &sco->main);
}

#endif//_H_SCOROUTINE$LINUX
