CC = gcc
CFLAGS = -Wall -Wextra -std=c99  # 编译选项（开启警告、C99 标准）
SRC = virtdisk.c bitmap.c ext2.c main.c   # 所有源文件
OBJ = $(SRC:.c=.o)
EXEC = simple_fs_test  # 生成的可执行文件名
all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(EXEC) $(OBJ)
run:$(EXEC)
	./simple_fs_test