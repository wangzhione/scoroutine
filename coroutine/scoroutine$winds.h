#if !defined(_H_SCOROUTINE$WINDS) && defined(_MSC_VER)
#define _H_SCOROUTINE$WINDS

#include "scoroutine.h"

#include <windows.h>

// 声明协程结构 和 协程管理器结构
struct sco {
    PVOID ctx;              // 当前协程运行的环境
    sco_f func;             // 协程体执行
    void * arg;             // 用户输入的参数
    int status;             // 当前协程运行状态 SCO_*
};

struct scomng {
    PVOID main;             // 当前主协程记录运行环境
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

    // 在当前线程环境中开启 winds 协程
    comng->main = ConvertThreadToFiberEx(NULL, FIBER_FLAG_FLOAT_SWITCH);
    return comng;
}

// 销毁一个协程对象
inline void sco_delete(struct sco * co) {
    DeleteFiber(co->ctx);
    free(co);
}

//
// sco_close - 关闭已经开启的协程系统函数
// sco      : sco_oepn 返回的当前协程中协程管理器
// return   : void
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

    // 切换当前协程系统变回默认的主线程, 关闭协程系统
    ConvertFiberToThread();
}

// 构建 struct sco 协程对象
inline struct sco * sco_new(sco_f func, void * arg) {
    struct sco * co = malloc(sizeof(struct sco));
    assert(co && func);
    co->func = func;
    co->arg = arg;
    co->status = SCO_READY;
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

static VOID WINAPI _sco_main(LPVOID ptr) {
    struct scomng * comng = ptr;
    int id = comng->running;
    struct sco * co = comng->cos[id];

    // 执行协程体
    co->func(comng, co->arg);
    co = comng->cos[id];
    co->status = SCO_DEAD;

    // 跳转到主纤程体中销毁
    SwitchToFiber(comng->main);
}

//
// sco_resume - 通过协程id激活协程
// sco      : 协程系统管理器
// id       : 具体协程id, sco_create 返回的协程id
// return   : void
//
void
sco_resume(scomng_t sco, int id) {
    int running;
    struct sco * co;

    assert(sco && id >= 0 && id < sco->cap);

    // SCO_DEAD 状态协程, 完全销毁其它协程操作
    running = sco->running;
    if (running != -1) {
        co = sco->cos[running];
        assert(co && co->status == SCO_DEAD);
        sco->cos[running] = NULL;
        --sco->cnt;
        sco->idx = running;
        sco->running = -1;
        sco_delete(co);
        if (running == id)
            return;
    }

    // 下面是协程 SCO_READY 和 SCO_SUSPEND 处理
    co = sco->cos[id];
    if ((!co) || (co->status != SCO_READY && co->status != SCO_SUSPEND))
        return;

    // Window特性创建纤程, 并保存当前上下文环境, 切换到创建的纤程环境中
    if (co->status == SCO_READY)
        co->ctx = CreateFiberEx(INT_STACK, 0, FIBER_FLAG_FLOAT_SWITCH, _sco_main, sco);

    co->status = SCO_RUNNING;
    sco->running = id;

    // 正常逻辑切换到创建的子纤程中
    sco->main = GetCurrentFiber();
    SwitchToFiber(co->ctx);
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

    co->status = SCO_SUSPEND;
    sco->running = -1;

    co->ctx = GetCurrentFiber();
    SwitchToFiber(sco->main);
}

#endif//_H_SCOROUTINE$WINDS
