OVMF_PATH=OVMFbin

c: 
	gcc -I gnu-efi/inc -fpic -ffreestanding \
	-fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -c main.c -o main.o

l: 
	ld -shared -Bsymbolic -L gnu-efi/x86_64/lib -L gnu-efi/x86_64/gnuefi \
	-T gnu-efi/gnuefi/elf_x86_64_efi.lds gnu-efi/x86_64/gnuefi/crt0-efi-x86_64.o main.o \
	-o main.so -lgnuefi -lefi

o: 
	objcopy -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym  -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 --subsystem=10 main.so main.efi

q: 
	mv main.efi img/efi/boot
	qemu-system-x86_64 -m 256M -cpu qemu64 -drive if=pflash,format=raw,unit=0,file="$(OVMF_PATH)/OVMF_CODE-pure-efi.fd",readonly=on \
	-drive if=pflash,format=raw,unit=1,file="$(OVMF_PATH)/OVMF_VARS-pure-efi.fd" -net none -drive file=fat:rw:img,format=raw -nographic 
	
