OVMF_PATH=OVMFbin
BL_PATH=bootloader
BOOT_PATH = img/efi/boot

compile: 
	gcc -I gnu-efi/inc -fpic -ffreestanding \
	-fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c "$(BL_PATH)/main.c" -o "$(BL_PATH)/main.o"
	ld -shared -Bsymbolic -L gnu-efi/x86_64/lib -L gnu-efi/x86_64/gnuefi \
	-T gnu-efi/gnuefi/elf_x86_64_efi.lds gnu-efi/x86_64/gnuefi/crt0-efi-x86_64.o "$(BL_PATH)/main.o" \
	-o "$(BL_PATH)/main.so" -lgnuefi -lefi
	objcopy -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 "$(BL_PATH)/main.so" "$(BL_PATH)/main.efi"

makeimg: compile
	mv "$(BL_PATH)/main.efi" BOOT_PATH
	qemu-system-x86_64 -m 256M -cpu qemu64 -drive if=pflash,format=raw,unit=0,file="$(OVMF_PATH)/OVMF_CODE-pure-efi.fd",readonly=on \
	-drive if=pflash,format=raw,unit=1,file="$(OVMF_PATH)/OVMF_VARS-pure-efi.fd" -net none -drive file=fat:rw:img,format=raw -nographic 
	
