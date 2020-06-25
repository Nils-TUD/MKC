c compile :
	make -C kern/build

QEMU = qemu-system-i386

r run :
	$(QEMU) -kernel kern/build/hypervisor -serial stdio -display none

cl clean :
	make -C kern/build clean

cla cleanall : clean
	make -C kern/build cleanall
