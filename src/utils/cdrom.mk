$(BUILD)/kernel.iso: $(BUILD)/kernel.bin \
					$(SRC)/utils/grub.cfg
# 检测内核文件是否合法
	grub-file --is-x86-multiboot2 $<
# 创建iso目录
	mkdir -p $(BUILD)/iso/boot/grub
# 拷贝内核文件
	cp $< $(BUILD)/iso/boot
# 拷贝 grub 配置文件
	cp $(SRC)/utils/grub.cfg $(BUILD)/iso/boot/grub
# 生成iso文件，报错需要安装 xorriso, mtools
	grub-mkrescue -o $@ $(BUILD)/iso 


.PHONY: bochsb
bochsb: $(BUILD)/kernel.iso 
	bochs -q -f ../bochs/bochsrc-grub -unlock
	
QEMU_CDROM:=-boot d \
	-drive file=$(BUILD)/kernel.iso,media=cdrom \

.PHONY: qemub
qemub: $(BUILD)/kernel.iso $(IMAGES)
	$(QEMU) $(QEMU_CDROM) \
	# $(QEMU_DEBUG)

.PHONY: cdrom
cdrom: $(BUILD)/kernel.iso $(IMAGES)