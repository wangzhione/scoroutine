# 全局替换变量
CC     = gcc 
CFLAGS = -g -Wall -O2 

# 声明路径变量
DTAR   = Debug

# 构建伪命令
.PHONY : all clean

# make 开始的标签
all : $(DTAR)/main.exe

$(DTAR)/main.exe : main.o coroutine.o
	$(CC) $(CFLAGS) -o $@ $(addprefix $(DTAR)/, $^)
	
%.o : %.c | $(DTAR)
	$(CC) $(CFLAGS) -c -o $(DTAR)/$@ $<

$(DTAR) :
	-mkdir -p $(DTAR)
	
# 清除操作
clean :
	-rm -rf $(DTAR) Release x64
