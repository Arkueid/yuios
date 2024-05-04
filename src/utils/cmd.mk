.PHONY: bochs
bochs: $(IMAGES)
	bochs -q -f ../bochs/bochsrc -unlock

.PHONY: bochsg
bochsg: $(IMAGES)
	bochs-gdb -q -f ../bochs/bochsrc-gdb -unlock

QEMU:= qemu-system-i386 \
	-m 32M \
	-rtc base=localtime \

QEMU+= -chardev stdio,mux=on,id=com1 # 字符设备 1
QEMU+= -serial chardev:com1 # 串口 1

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

.PHONY: clean
clean:
	rm -rf $(BUILD)