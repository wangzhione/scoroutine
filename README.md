## coroutine 跨平台协程玩耍库

    * main.c              --  测试主逻辑
    * scoroutine.h        --  协程库接口
    * scoroutine.c        --  协程库实现
    * scoroutine$linux.h  --  linux 局部实现
    * scoroutine$winds.h  --  winds 局部实现

### 编译说明

    Winds 使用 Best new CL 操作 sln 文件就可以
    Linux 上 need GCC , 随后 Makefile 就 OK
  
### 扯淡这是

    采用 $ 命名文件.
    意图是, 表明当前文件是私有(局部)的, 不推荐使用.
    而 $ 字符是 C 中变量命名一个"特殊"字符, 比 _ __ or - 这些, 要更贴合编译器.
    
    scoroutine 协程库比较简单, 适合学习了解这个编程的 `协程` 时代

最初的思路来自 -> 
[coroutine](https://github.com/cloudwu/coroutine)

***

#### 还是写代码有意思

***

[二十年](http://music.163.com/m/song?id=193108&userid=16529894)

    (:) 0.o
