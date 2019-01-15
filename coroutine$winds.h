#if  !defined(COROUTINE$WINDS_H) && defined(_WIN32)
#define COROUTINE$WINDS_H

#include <windows.h>
#include "coroutine.h"

// 声明 co - 协程结构 和 comng - 协程管理器结构
struct co {
    co_f func;              // 协程执行行为
    void * arg;             // 协程执行参数
    int status;             // 当前协程状态 CO_*
    void * ctx;             // 当前协程运行的上下文环境
};

// co_new - 构建 struct co 协程对象
inline struct co * co_new(co_f func, void * arg) {
    struct co * c = malloc(sizeof(struct co));
    assert(c && func);
    c->func = func;
    c->arg = arg;
    c->status = CO_READY;
    c->ctx = NULL;
    return c;
}

// co_delete - 销毁一个协程对象
inline void co_die(struct co * c) {
    if (c->ctx)
        DeleteFiber(c->ctx);
    free(c);
}

struct comng {
    struct co ** cs;        // 协程对象集, 循环队列
    int cap;                // 协程对象集容量
    int idx;                // 当前协程集中轮询到的索引
    int len;                // 当前协程集存在的协程个数

    int running;            // 当前协程集中运行的协程 id
    void * ctx;             // 当前主协程记录运行上下文环境
};

// comng_run - 协程管理器运行实体
static void __stdcall comng_run(struct comng * g) {
    int id = g->running;
    struct co * c = g->cs[id];

    // 执行协程体
    c->func(g, c->arg);
    c->status = CO_DEAD;

    // 跳转到主纤程体中销毁
    SwitchToFiber(g->ctx);
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

    // 在当前线程环境中开启 winds 协程
    g->ctx = ConvertThreadToFiberEx(NULL, FIBER_FLAG_FLOAT_SWITCH);
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

    // 切换当前协程系统变回默认的主线程, 关闭协程系统
    ConvertFiberToThread();
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
    assert(g && id >= 0 && id < g->cap);

    // CO_DEAD 状态协程, 完全销毁其它协程操作
    int running = g->running;
    if (running != -1) {
        struct co * c = g->cs[running];
        assert(c && c->status == CO_DEAD);

        g->cs[running] = NULL;
        --g->len;
        g->idx = running;
        g->running = -1;
        co_die(c);
        if (running == id)
            return;
    }

    // 下面是协程 CO_READY 和 CO_SUSPEND 处理
    struct co * c = g->cs[id];
    if ((!c) || (c->status != CO_READY && c->status != CO_SUSPEND))
        return;

    // window 创建纤程, 并保存当前上下文环境
    if (c->status == CO_READY)
        c->ctx = CreateFiberEx(STACK_INT, 0, FIBER_FLAG_FLOAT_SWITCH, comng_run, g);

    c->status = CO_RUNNING;
    g->running = id;

    // 正常逻辑切换到创建的子纤程上下文环境中
    g->ctx = GetCurrentFiber();
    SwitchToFiber(c->ctx);
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
    c->status = CO_SUSPEND;
    g->running = -1;

    c->ctx = GetCurrentFiber();
    SwitchToFiber(g->ctx);
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

#endif//COROUTINE$WINDS_H
