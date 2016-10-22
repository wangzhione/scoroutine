# 全局替换变量
CC = gcc 
CFLAGS = -g -Wall -O2 
RUNE = $(CC) $(CFLAGS) -o $@ $^

# 声明路径变量
SRC_PATH := ./coroutine
TAR_PATH := ./Debug

# 构建伪命令
.PHONY:all clean cleanall

# 第一个标签, 是make的开始
all:$(TAR_PATH)/main.out

$(TAR_PATH)/main.out:main.o scoroutine.o
	$(CC) $(CFLAGS) -o $@ $(addprefix $(TAR_PATH)/, $^ )

$(TAR_PATH):
	mkdir $@
	
%.o:$(SRC_PATH)/%.c | $(TAR_PATH)
	$(CC) $(CFLAGS) -c -o $(TAR_PATH)/$@ $<
	
# 清除操作
clean:
	-rm -rf $(TAR_PATH)/*.i $(TAR_PATH)/*.s $(TAR_PATH)/*.o $(TAR_PATH)/*~
cleanall:clean
	-rm -rf $(TAR_PATH)
