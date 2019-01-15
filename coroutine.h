#ifndef COROUTINE_H
#define COROUTINE_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

// 默认协程栈大小 和 初始化协程数量
#define STACK_INT       (1048576)
#define COROUTINE_INT   (16)

#define CO_DEAD         (0) // 协程死亡状态
#define CO_READY        (1) // 协程已经就绪
#define CO_RUNNING      (2) // 协程正在运行
#define CO_SUSPEND      (3) // 协程暂停等待

// comng_t - 协程管理器对象
typedef struct comng * comng_t;

// co_f - 协程运行函数体
// g        : 协程管理器对象
// arg      : 用户创建协程传入参数
typedef void (* co_f)(comng_t g, void * arg);

//
// co_open - 开启协程系统, 返回协程管理器对象
// return   : 创建的协程管理器对象
//
extern comng_t co_open(void);

//
// co_close - 关闭开启的协程系统
// g        : 协程管理器对象
// return   : void
//
extern void co_close(comng_t g);

//
// co_create - 创建一个就绪态协程
// g        : 协程管理器对象
// func     : 协程体执行的函数体
// arg      : 协程体中传入的参数
// return   : 返回协程 id
//
extern int co_create(comng_t g, co_f func, void * arg);

//
// co_resume - 通过协程 id 激活协程
// g        : 协程管理器对象
// id       : 协程id
// return   : void
//
extern void co_resume(comng_t g, int id);

//
// co_yield - 暂停正在运行的协程
// g        : 协程管理器对象
// return   : void
//
extern void co_yield(comng_t g);

//
// co_running - 获取正在运行的协程 id
// g        : 协程系统管理器
// retrunr  : < 0 表示没有协程在运行
//
extern int co_running(comng_t g);

//
// co_status - 获取对应协程的状态
// g        : 协程管理器对象
// id       : 协程 id
// return   : 协程状态
//
extern int co_status(comng_t g, int id);

#endif//COROUTINE_H
