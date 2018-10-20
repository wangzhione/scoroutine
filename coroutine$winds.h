#if  !defined(_H_COROUTINE$WINDS) && defined(_MSC_VER)
#define _H_COROUTINE$WINDS

#include <windows.h>
#include "coroutine.h"

// co - 声明协程结构 和 cg - 协程管理器结构
struct co {
    co_f func;              // 协程体执行
    void * arg;             // 用户输入的参数
    int status;             // 当前协程运行状态 CO_*
    void * ctx;             // 当前协程运行的环境
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
    int cnt;                // 当前存在的协程个数

    int running;            // 当前协程中运行的协程 id
    void * ctx;             // 当前主协程记录运行环境
};

// comng_run - 协程管理器运行实体
static void __stdcall comng_run(struct comng * cg) {
    int id = cg->running;
    struct co * c = cg->cs[id];

    // 执行协程体
    c->func(cg, c->arg);
    c->status = CO_DEAD;

    // 跳转到主纤程体中销毁
    SwitchToFiber(cg->ctx);
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

    // 在当前线程环境中开启 winds 协程
    cg->ctx = ConvertThreadToFiberEx(NULL, FIBER_FLAG_FLOAT_SWITCH);
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

    // 切换当前协程系统变回默认的主线程, 关闭协程系统
    ConvertFiberToThread();
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
    int running;
    struct co * c;
    assert(cg && id >= 0 && id < cg->cap);

    // CO_DEAD 状态协程, 完全销毁其它协程操作
    running = cg->running;
    if (running != -1) {
        c = cg->cs[running];
        assert(c && c->status == CO_DEAD);

        cg->cs[running] = NULL;
        --cg->cnt;
        cg->idx = running;
        cg->running = -1;
        co_die(c);
        if (running == id)
            return;
    }

    // 下面是协程 CO_READY 和 CO_SUSPEND 处理
    c = cg->cs[id];
    if ((!c) || (c->status != CO_READY && c->status != CO_SUSPEND))
        return;

    // Window特性创建纤程, 并保存当前上下文环境, 切换到创建的纤程环境中
    if (c->status == CO_READY)
        c->ctx = CreateFiberEx(INT_STACK, 0, FIBER_FLAG_FLOAT_SWITCH, comng_run, cg);

    c->status = CO_RUNNING;
    cg->running = id;

    // 正常逻辑切换到创建的子纤程中
    cg->ctx = GetCurrentFiber();
    SwitchToFiber(c->ctx);
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

    c->status = CO_SUSPEND;
    cg->running = -1;

    c->ctx = GetCurrentFiber();
    SwitchToFiber(cg->ctx);
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
