c compile :
	make -C kern/build

QEMU = qemu-system-i386
QEMU_ARGS = -kernel kern/build/hypervisor -serial stdio -display none

r run : compile
	$(QEMU) $(QEMU_ARGS)

d dbg : compile
	$(QEMU) $(QEMU_ARGS) -S -s &
	gdb --tui kern/build/hypervisor --init-eval-command="target remote localhost:1234"
	killall $(QEMU)

cl clean :
	make -C kern/build clean

cla cleanall : clean
	make -C kern/build cleanall
