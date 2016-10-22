#include <stdio.h>
#include "scoroutine.h"

#define _INT_TEST	(5)

struct args {
	int n;
};

static void _foo(void * sco, void * arg) {
	struct args * as = arg;
	int start = as->n;
	int i = -1;

	while (++i < _INT_TEST) {
		printf("coroutine %d : %d.\n", sco_running(sco), start + i);
		sco_yield(sco);
	}
}

static void _test(void * sco) {
	struct args argo = { 000 };
	struct args argt = { 100 };

	int coo = sco_create(sco, _foo, &argo);
	int cot = sco_create(sco, _foo, &argt);

	puts("********************_test start********************");
	while (sco_status(sco, coo) && sco_status(sco, cot)) {
		sco_resume(sco, coo);
		sco_resume(sco, cot);
	}
	puts("********************_test e n d********************");
}

/*
 * 主逻辑, 主要测试多个协程之间的纠缠
 */
int main(void) {
	void * sco = sco_open();
	
	puts("--------------------突然想起了什么,--------------------\n");
	_test(sco);

	// 再来测试一下, 纤程切换问题
	struct args arg = { 222 };
	int co = sco_create(sco, _foo, &arg);
	for (int i = -1; i < _INT_TEST; ++i)
		sco_resume(sco, co);

	puts("\n--------------------笑了笑, 我自己.--------------------");
	sco_close(sco);
	return 0;
}
