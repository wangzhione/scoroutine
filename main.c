#include <stdio.h>
#include "coroutine.h"

#define INT_TEST    (5)

struct args { int n; };

static void run(comng_t cg, struct args * arg) {
    int start = arg->n;
    for (int i = 0; i < INT_TEST; ++i) {
        printf("coroutine %d : %d\n", co_running(cg), start + i);
        co_yield(cg);
    }

    printf("coroutine %d : %d\n", co_running(cg), start);
}

static void test(comng_t cg) {
    struct args aro = {   0 };
    struct args art = { 111 };

    int coo = co_create(cg, (co_f)run, &aro);
    int cot = co_create(cg, (co_f)run, &art);

    puts("********************** test start **********************");
    while (co_status(cg, coo) && co_status(cg, cot)) {
        co_resume(cg, coo);
        co_resume(cg, cot);
    }
    puts("********************** test e n d **********************");
}

//
// 主逻辑, 用于测试多个协程之间的纠缠
//
int main(void) {
    comng_t cg = co_open();

    puts("-------------------- 突然想起了什么, --------------------\n");

    test(cg);

    // 再来测试一下, 纤程切换问题
    struct args arg = { 222 };
    int id = co_create(cg, (co_f)run, &arg);
    for (int i = -1; i < INT_TEST; ++i)
        co_resume(cg, id);

    puts("\n-------------------- 笑了笑, 我自己. --------------------");
    
    co_close(cg);
    return EXIT_SUCCESS;
}
