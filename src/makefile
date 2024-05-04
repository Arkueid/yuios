# 构建文件夹
BUILD:=../build
# 源文件夹
SRC:=.

CFLAGS:= -m32 						# 32位的程序
CFLAGS+= -Qn						# 去掉GCC版本信息
CFLAGS+= -fno-builtin 				# 不要 gcc 内置函数
CFLAGS+= -nostdinc 					# 不要标准头文件
CFLAGS+= -fno-pic 					# 不要位置无关的代码
CFLAGS+= -fno-pie 					# 不要位置无关的可执行程序
CFLAGS+= -nostdlib 					# 不要标准库
CFLAGS+= -fno-stack-protector 		# 不要栈保护
CFLAGS+= -fomit-frame-pointer		# 不需要栈帧
CFLAGS+= -fno-asynchronous-unwind-tables # 不需要调试信息
CFLAGS+= -mpreferred-stack-boundary=2 # 不要栈对齐
# 除去空白字符，不除去 CFLAGS="-nostdinc	"
CFLAGS:=$(strip ${CFLAGS})

DEBUG:= -g
# 头文件目录
INCLUDE:= -I$(SRC)/include

MULTIBOOT2:=0x10000
ENTRYPOINT:=$(shell python -c "print(f'0x{$(MULTIBOOT2) + 64:x}')")

$(BUILD)/boot/%.bin: $(SRC)/boot/%.asm
	$(shell mkdir -p $(dir $@))
	nasm -f bin $< -o $@

$(BUILD)/%.o: $(SRC)/%.asm
	$(shell mkdir -p $(dir $@))
	nasm -f elf32 $(DEBUG) $< -o $@

# c文件 生成 可重定位文件
$(BUILD)/%.o: $(SRC)/%.c
	$(shell mkdir -p $(dir $@))
	gcc $(CFLAGS) $(DEBUG) $(INCLUDE) -c $< -o $@

LDFLAGS:= -m elf_i386 \
			-static \
			-Ttext $(ENTRYPOINT) \
			--section-start=.multiboot2=$(MULTIBOOT2)
LDFLAGS:=$(strip ${LDFLAGS})

# 链接
$(BUILD)/kernel.bin: $(BUILD)/kernel/start.o \
	$(BUILD)/kernel/main.o \
	$(BUILD)/kernel/io.o \
	$(BUILD)/kernel/device.o \
	$(BUILD)/kernel/console.o \
	$(BUILD)/kernel/printk.o \
	$(BUILD)/kernel/assert.o \
	$(BUILD)/kernel/debug.o \
	$(BUILD)/kernel/global.o \
	$(BUILD)/kernel/task.o \
	$(BUILD)/kernel/schedule.o \
	$(BUILD)/kernel/handler.o \
	$(BUILD)/kernel/interrupt.o \
	$(BUILD)/kernel/clock.o \
	$(BUILD)/kernel/time.o \
	$(BUILD)/kernel/rtc.o \
	$(BUILD)/kernel/memory.o \
	$(BUILD)/kernel/gate.o \
	$(BUILD)/kernel/thread.o \
	$(BUILD)/kernel/mutex.o \
	$(BUILD)/kernel/keyboard.o \
	$(BUILD)/kernel/arena.o \
	$(BUILD)/kernel/ide.o \
	$(BUILD)/kernel/buffer.o \
	$(BUILD)/kernel/system.o \
	$(BUILD)/lib/stdlib.o \
	$(BUILD)/lib/string.o \
	$(BUILD)/lib/vsprintf.o \
	$(BUILD)/lib/bitmap.o \
	$(BUILD)/lib/syscall.o \
	$(BUILD)/lib/list.o  \
	$(BUILD)/lib/fifo.o  \
	$(BUILD)/lib/printf.o  \
	$(BUILD)/fs/super.o  \
	$(BUILD)/fs/bmap.o  \
	$(BUILD)/fs/inode.o  \
	$(BUILD)/fs/namei.o  \
	$(BUILD)/fs/file.o  \
	$(BUILD)/fs/stat.o  \
	$(BUILD)/builtin/osh.o  \

	$(shell mkdir -p $(dir $@))
	ld $(LDFLAGS) $^ -o $@


$(BUILD)/system.bin: $(BUILD)/kernel.bin
	objcopy -O binary $< $@

# 符号表
$(BUILD)/system.map: $(BUILD)/kernel.bin
	nm $< | sort > $@

include utils/image.mk
include utils/cdrom.mk
include utils/cmd.mk




