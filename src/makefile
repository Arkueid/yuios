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
	$(BUILD)/lib/stdlib.o \
	$(BUILD)/lib/string.o \
	$(BUILD)/lib/vsprintf.o \
	$(BUILD)/lib/bitmap.o \
	$(BUILD)/lib/syscall.o \
	$(BUILD)/lib/list.o  \
	$(BUILD)/lib/fifo.o  \
	$(BUILD)/lib/printf.o  

	$(shell mkdir -p $(dir $@))
	ld $(LDFLAGS) $^ -o $@


$(BUILD)/system.bin: $(BUILD)/kernel.bin
	objcopy -O binary $< $@

# 符号表
$(BUILD)/system.map: $(BUILD)/kernel.bin
	nm $< | sort > $@

$(BUILD)/master.img: $(BUILD)/boot/boot.bin \
		$(BUILD)/boot/loader.bin \
		$(BUILD)/system.bin \
		$(BUILD)/system.map \
		$(SRC)/utils/master.sfdisk \

	yes | bximage -q -hd=16 -func=create -sectsize=512 -imgmode=flat $@
	dd if=$(BUILD)/boot/boot.bin of=$@ bs=512 count=1 conv=notrunc
	dd if=$(BUILD)/boot/loader.bin of=$@ bs=512 count=4 seek=2 conv=notrunc

	test -n "$$(find $(BUILD)/system.bin -size -100k)"
	
	dd if=$(BUILD)/system.bin of=$@ bs=512 count=200 seek=10 conv=notrunc

# 对硬盘进行分区
	sfdisk $@ < $(SRC)/utils/master.sfdisk

$(BUILD)/slave.img:
	yes | bximage -q -hd=32 -func=create -sectsize=512 -imgmode=flat $@

IMAGES:=$(BUILD)/master.img $(BUILD)/slave.img

image: $(IMAGES)

.PHONY: clean
clean:
	rm -rf $(BUILD)

.PHONY: bochs
bochs: $(IMAGES)
	bochs -q -f ../bochs/bochsrc -unlock

.PHONY: bochsg
bochsg: $(IMAGES)
	bochs-gdb -q -f ../bochs/bochsrc-gdb -unlock

test: $(IMAGES)


QEMU:= qemu-system-i386 \
	-m 32M \
	-rtc base=localtime \

QEMU_DISK:=-boot c \
	-drive file=$(BUILD)/master.img,if=ide,index=0,media=disk,format=raw \
	-drive file=$(BUILD)/slave.img,if=ide,index=1,media=disk,format=raw \

QEMU_DEBUG:= -s -S

.PHONY: qemu
qemu: $(IMAGES)
	$(QEMU) $(QEMU_DISK)

.PHONY: qemug
qemug: $(IMAGES)
	$(QEMU) $(QEMU_DISK) $(QEMU_DEBUG)

$(BUILD)/master.vmdk: $(BUILD)/master.img
	qemu-img convert -O vmdk $< $@

.PHONY: vmdk
vmdk: $(BUILD)/master.vmdk