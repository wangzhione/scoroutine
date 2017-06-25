## scoroutine 一个跨平台协程库

    * main.c              --  测试代码
    * scoroutine.h        --  协程库接口
    * scoroutine.c        --  协程库实现
    * scoroutine$linux.h  --  linux上实现协程库特定接口
    * scoroutine$winds.h  --  winds上实现协程库特定接口

## 编译说明
    Winds 使用 VS2015/VS2017 打开 sln 文件就可以, 推荐用Best new cl and nmae
    Linux 上 need GCC , 随后 Makefile 就可以使用了
  
## 补充扯淡
    采用 $ 命名文件, 意图就是表示 , 当前文件是私有(局部)的文件, 不推荐使用.
    而 $ 字符是C中变量命名一个"特殊"字符, 个人感觉比 __ or - 这些命令要好看些.
    
    更加详细版本 可以追踪下面地址 * 在 simplec scoroutine 协程库中更新
    
>>>[simplec scoroutine](https://github.com/wangzhione/simplec/blob/master/simplec/module/service/include/scoroutine.h)
