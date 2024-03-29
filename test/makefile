CFLAGS:= -m32 						# 32位的程序
CFLAGS+= -Qn						# 去掉GCC版本信息
CFLAGS+= -fno-builtin 				# 不要 gcc 内置函数
CFLAGS+= -nostdinc 					# 不要标准头文件
CFLAGS+= -fno-pic 					# 不要位置无关的代码
CFLAGS+= -fno-pie 					# 不要位置无关的可执行程序
CFLAGS+= -nostdlib 					# 不要标准库
CFLAGS+= -fno-stack-protector 		# 不要栈保护
# CFLAGS+= -fomit-frame-pointer		# 不需要栈帧
CFLAGS+= -fno-asynchronous-unwind-tables # 不需要调试信息
CFLAGS+= -mpreferred-stack-boundary=2 # 不要栈对齐
# 除去空白字符，不除去 CFLAGS="-nostdinc	"
CFLAGS:=$(strip ${CFLAGS})

DEBUG:= -g

%.o: %.c
	gcc $(CFLAGS) $< -o $@

.PHONY: test
test: stack.o

.PHONY: clean
clean:
	rm -rf *.s
	rm -rf *.o
	rm -rf *.out