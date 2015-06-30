c compile :
	make -C kern/build

QEMU = qemu-system-i386

r run :
	cd boot && $(QEMU) -net nic,model=ne2k_pci -net user -fda grub_disk -tftp . -display sdl -serial stdio

cl clean :
	make -C kern/build clean

cla cleanall : clean
	make -C kern/build cleanall
