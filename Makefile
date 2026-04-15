c compile :
	make -C kern/build

QEMU = qemu-system-x86_64
ISO_DIR = dist/iso
ISO = dist/nova.iso
QEMU_ARGS = -cdrom $(ISO) -boot d -serial stdio -display none -no-reboot

iso : compile
	mkdir -p $(ISO_DIR)/boot/grub
	cp kern/build/hypervisor $(ISO_DIR)/boot/hypervisor
	cp boot/grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(ISO_DIR)

r run : iso
	$(QEMU) $(QEMU_ARGS)

d dbg : iso
	$(QEMU) $(QEMU_ARGS) -S -s &
	gdb --tui kern/build/hypervisor --init-eval-command="target remote localhost:1234"
	killall $(QEMU)

cl clean :
	make -C kern/build clean
	rm -rf dist

cla cleanall : clean
	make -C kern/build cleanall
