#ifndef _H_COROUTINE
#define _H_COROUTINE

#define CO_DEAD     (0) // 协程死亡状态
#define CO_READY    (1) // 协程已经就绪
#define CO_RUNNING  (2) // 协程正在运行
#define CO_SUSPEND  (3) // 协程暂停等待

// comng_t - 协程管理对象
typedef struct comng * comng_t;

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
// sco      : 协程管理对象
// return   : void
//
extern void co_close(comng_t cg);

#endif//_H_COROUTINE
