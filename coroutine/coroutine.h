#ifndef _H_COROUTINE
#define _H_COROUTINE

// comng_t - 协程管理对象
typedef struct comng * comng_t;

#define CO_DEAD     (0) // 协程死亡状态
#define CO_READY    (1) // 协程已经就绪
#define CO_RUNNING  (2) // 协程正在运行
#define CO_SUSPEND  (3) // 协程暂停等待

// co_f - 协程运行函数体
// cg   : 协程管理对象
// arg  : 用户创建协程传入参数
typedef void (* co_f)(comng_t cg, void * arg);

//
// co_open - 开启协程系统, 并返回创建的协程管理对象
// return  : 创建的协程管理对象
//
extern comng_t co_open(void);

//
// co_close - 关闭已经开启的协程系统
// cg      : 协程管理对象
// return   : void
//
extern void co_close(comng_t cg);

//
// co_create - 创建一个协程, 此刻是就绪态
// cg        : 协程管理对象
// func      : 协程体执行的函数体
// arg       : 协程体中传入的参数
// return    : 返回创建好的协程 id
//
extern int co_create(comng_t cg, co_f func, void * arg);

//
// co_resume - 通过协程 id 激活协程
// cg        : 协程管理对象
// id        : 协程id
// return    : void
//
extern void co_resume(comng_t cg, int id);

//
// co_yield - 关闭当前正在运行的协程, 协程暂停
// cg       : 协程管理对象
// return   : void
//
extern void co_yield(comng_t cg);

//
// co_running - 当前协程系统中运行的协程 id
// cg         : 协程系统管理器
// retrunr    : 返回 < 0 表示没有协程在运行
//
extern int co_running(comng_t cg);

//
// co_status - 得到当前协程状态
// cg        : 协程管理对象
// id        : 协程 id
// return    : 返回对应协程状态信息
//
extern int co_status(comng_t cg, int id);

#endif//_H_COROUTINE
