c compile :
	make -C kern/build

ISO = kern/build/nova.iso
GRUB_CFG = kern/build/iso/boot/grub/grub.cfg

$(ISO) : compile
	mkdir -p kern/build/iso/boot/grub
	cp kern/build/hypervisor kern/build/iso/boot/hypervisor
	printf 'set timeout=0\nset default=0\nmenuentry "NOVA" {\n  multiboot2 /boot/hypervisor\n  boot\n}\n' > $(GRUB_CFG)
	grub-mkrescue -o $(ISO) kern/build/iso 2>/dev/null

QEMU = qemu-system-x86_64
QEMU_ARGS = -cdrom $(ISO) -serial stdio -display none -no-reboot

r run : $(ISO)
	$(QEMU) $(QEMU_ARGS)

d dbg : $(ISO)
	$(QEMU) $(QEMU_ARGS) -S -s &
	gdb --tui kern/build/hypervisor --init-eval-command="target remote localhost:1234"
	killall $(QEMU)

cl clean :
	make -C kern/build clean
	rm -rf kern/build/iso $(ISO)

cla cleanall : clean
	make -C kern/build cleanall
