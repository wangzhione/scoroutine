# 全局替换变量
CC     = gcc 
CFLAGS = -g -Wall -O2 
RUNE   = $(CC) $(CFLAGS) -o $@ $^

# 声明路径变量
DSRC   = coroutine
DTAR   = Debug

# 构建伪命令
.PHONY : all clean

# 第一个标签, 是 make 的开始
all : $(DTAR)/main.exe

$(DTAR)/main.exe : main.o scoroutine.o
	$(CC) $(CFLAGS) -o $@ $(addprefix $(DTAR)/, $^)
	
%.o : $(DSRC)/%.c | $(DTAR)
	$(CC) $(CFLAGS) -c -o $(DTAR)/$@ $<

$(DTAR) :
	-mkdir -p $(DTAR)
	
# 清除操作
clean :
	-rm -rf $(DTAR) Release x64
	-rm -rf $(DSRC)/Debug $(DSRC)/Release $(DSRC)/x64
