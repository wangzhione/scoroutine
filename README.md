# coroutine 协程库

    * main.c            --  测试主逻辑
    * coroutine.h       --  协程库接口
    * coroutine.c       --  协程库实现
    * coroutine$linux.h --  linux 局部实现
    * coroutine$winds.h --  winds 局部实现

## 编译说明

    winds 使用 best new vs 操作 sln
    linux 上 need gcc , 随后 make 就 OK ~

```
$ make
mkdir -p Debug
gcc  -g -Wall -O2  -c -o Debug/main.o main.c
gcc  -g -Wall -O2  -c -o Debug/coroutine.o coroutine.c
gcc  -g -Wall -O2  -o Debug/main.exe Debug/main.o Debug/coroutine.o

$ ./Debug/main.exe
-------------------- 突然想起了什么, --------------------

********************** test start **********************
coroutine 0 : 0
coroutine 1 : 111
coroutine 0 : 1
coroutine 1 : 112
coroutine 0 : 2
coroutine 1 : 113
coroutine 0 : 3
coroutine 1 : 114
coroutine 0 : 4
coroutine 1 : 115
coroutine 0 : 0
coroutine 1 : 111
********************** test e n d **********************
coroutine 1 : 222
coroutine 1 : 223
coroutine 1 : 224
coroutine 1 : 225
coroutine 1 : 226
coroutine 1 : 222

-------------------- 笑了笑, 我自己. --------------------

$ make clean
rm -rf Debug Release x64
```  

## 杂谈

    采用 $ 命名文件，
    意图表明当前文件是私有(局部)的， 不推荐使用.
    而 $ 字符可以用于 C 变量命名， 比 _ __ or - 这些， 要更贴合编译器.
    
    coroutine 协程库比较简单, 适合学习和了解这个 '协程' 时代

最初的思路来自 -> [coroutine](https://github.com/cloudwu/coroutine)

***

### 代码有点意思

***

[二十年](http://music.163.com/m/song?id=193108)

    (:) 0.o 