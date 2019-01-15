#include <stdio.h>
#include "coroutine.h"

#define TEST_INT    (5)

struct args { int n; };

static void run(comng_t g, struct args * arg) {
    int start = arg->n;
    for (int i = 0; i < TEST_INT; ++i) {
        printf("coroutine %d : %d\n", co_running(g), start + i);
        co_yield(g);
    }

    printf("coroutine %d : %d\n", co_running(g), start);
}

static void test(comng_t g) {
    struct args aro = {   0 };
    struct args art = { 111 };

    int coo = co_create(g, (co_f)run, &aro);
    int cot = co_create(g, (co_f)run, &art);

    puts("********************** test start **********************");
    while (co_status(g, coo) && co_status(g, cot)) {
        co_resume(g, coo);
        co_resume(g, cot);
    }
    puts("********************** test e n d **********************");
}

//
// main 主逻辑, 用于测试多个协程之间的纠缠
//
int main(void) {
    comng_t g = co_open();

    puts("-------------------- 突然想起了什么, --------------------\n");

    test(g);

    // 再来测试一下, 纤程切换问题
    struct args arg = { 222 };
    int id = co_create(g, (co_f)run, &arg);
    for (int i = -1; i < TEST_INT; ++i)
        co_resume(g, id);

    puts("\n-------------------- 笑了笑, 我自己. --------------------");
    
    co_close(g);

    return EXIT_SUCCESS;
}
