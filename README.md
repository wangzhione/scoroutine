# scoroutine
## 一个跨平台协程库
<br />
main.c              --  测试代码<br />
scoroutine.h        --  协程库接口<br />
scoroutine.c        --  协程库跨平台实现<br />
scoroutine$linx.h   --  linux上实现协程库接口<br />
scoroutine$winds.h  --  windows上实现协程库接口<br />
<br />
## 编译说明
  Window 使用 VS2015 打开 sln 文件就可以<br />
  GCC 就是 Makefile 就可以<br />
  
## 补充扯淡
  采用 $ 命名 文件, 意图就是表示 , 当前文件是私有文件, 不推荐使用.<br />
而 $ 字符是C中, 变量命名一个"特殊"字符. <br />


