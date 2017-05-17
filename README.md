# scoroutine
## 一个跨平台协程库

* main.c              --  测试代码
* scoroutine.h        --  协程库接口
* scoroutine.c        --  协程库跨平台实现
* scoroutine$linx.h   --  linux上实现协程库接口
* scoroutine$winds.h  --  windows上实现协程库接口

## 编译说明
    Window 使用 VS2015 打开 sln 文件就可以
    GCC 就是 Makefile 就可以
  
## 补充扯淡
    采用 $ 命名 文件, 意图就是表示 , 当前文件是私有文件, 不推荐使用.
    而 $ 字符是C中, 变量命名一个"特殊"字符. 
    更加详细版本 可以追踪下面地址 * 在 simplec scoroutine 协程库中更新
      -> https://github.com/wangzhione/simplec/blob/master/simplec/module/service/include/scoroutine.h
  

