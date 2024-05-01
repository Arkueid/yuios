$(BUILD)/master.img: $(BUILD)/boot/boot.bin \
		$(BUILD)/boot/loader.bin \
		$(BUILD)/system.bin \
		$(BUILD)/system.map \
		$(SRC)/utils/master.sfdisk \

# 创建一个 16M 的硬盘镜像
	yes | bximage -q -hd=16 -func=create -sectsize=512 -imgmode=flat $@

# 将 boot.bin 写入磁盘 0 号扇区
	dd if=$(BUILD)/boot/boot.bin of=$@ bs=512 count=1 conv=notrunc

# 将 loader.bin 写入硬盘
	dd if=$(BUILD)/boot/loader.bin of=$@ bs=512 count=4 seek=2 conv=notrunc

# 测试 system.bin 小于 100k，否则修改下一条语命令的 count
# 用于开发过程中检查系统的大小，防止系统写入硬盘镜像的时候被截断
	test -n "$$(find $(BUILD)/system.bin -size -100k)"
	
# 将 system.bin 写入硬盘
	dd if=$(BUILD)/system.bin of=$@ bs=512 count=200 seek=10 conv=notrunc

# 对硬盘进行分区
	sfdisk $@ < $(SRC)/utils/master.sfdisk

# 挂载设备
	sudo losetup /dev/loop0 --partscan $@

# 创建 minux 文件系统
	sudo mkfs.minix -1 -n 14 /dev/loop0p1

# 挂载文件系统
	sudo mount /dev/loop0p1 /mnt

# 切换所有者
	sudo chown $(USER) /mnt

# 创建目录
	mkdir -p /mnt/home
	mkdir -p /mnt/d1/d2/d3/d4

# 创建文件
	echo "hello yuios!!!, from root directory file..." > /mnt/hello.txt
	echo "hello yuios!!!, from home directory file..." > /mnt/home/hello.txt

# 卸载文件系统
	sudo umount /mnt

# 卸载设备
	sudo losetup -d /dev/loop0


$(BUILD)/slave.img:
# 写一个 32M 的硬盘镜像
	yes | bximage -q -hd=32 -func=create -sectsize=512 -imgmode=flat $@

# 执行硬盘分区
	sfdisk $@ < $(SRC)/utils/slave.sfdisk

# 挂载设备
	sudo losetup /dev/loop0 --partscan $@

# 创建 minux 文件系统
	sudo mkfs.minix -1 -n 14 /dev/loop0p1

# 挂载文件系统
	sudo mount /dev/loop0p1 /mnt

# 切换所有者
	sudo chown $(USER) /mnt

# 创建文件
	echo "slave root directory file..." > /mnt/hello.txt

# 卸载文件系统
	sudo umount /mnt

# 卸载设备
	sudo losetup -d /dev/loop0

.PHONY: mount0
mount0: $(BUILD)/master.img
	sudo losetup /dev/loop0 --partscan $<
	sudo mount /dev/loop0p1 /mnt
	sudo chown $(USER) /mnt

.PHONY: unmount0
unmount0: /dev/loop0
	-sudo umount /mnt
	-sudo losetup -d $<

.PHONY: mount1
mount1: $(BUILD)/slave.img
	sudo losetup /dev/loop1 --partscan $<
	sudo mount /dev/loop1p1 /mnt
	sudo chown $(USER) /mnt

.PHONY: unmount1
unmount1: $(BUILD)/slave.img
	-sudo umount /mnt
	-sudo losetup -d $<

IMAGES:=$(BUILD)/master.img $(BUILD)/slave.img

image: $(IMAGES)