#include <efi.h>
#include <efilib.h>
#include <elf.h>

#define PAGE_SIZE 0x1000

const CHAR16* EfiStatusToStr(EFI_STATUS s) {
    switch (s) {
        case EFI_SUCCESS:
            return L"EFI_SUCCESS";
        case EFI_LOAD_ERROR:
            return L"EFI_LOAD_ERROR";
        case EFI_INVALID_PARAMETER:
            return L"EFI_INVALID_PARAMETER";
        case EFI_UNSUPPORTED:
            return L"EFI_UNSUPPORTED";
        case EFI_BAD_BUFFER_SIZE:
            return L"EFI_BAD_BUFFER_SIZE";
        case EFI_BUFFER_TOO_SMALL:
            return L"EFI_BUFFER_TOO_SMALL";
        case EFI_NOT_READY:
            return L"EFI_NOT_READY";
        case EFI_DEVICE_ERROR:
            return L"EFI_DEVICE_ERROR";
        case EFI_WRITE_PROTECTED:
            return L"EFI_WRITE_PROTECTED";
        case EFI_OUT_OF_RESOURCES:
            return L"EFI_OUT_OF_RESOURCES";
        case EFI_VOLUME_CORRUPTED:
            return L"EFI_VOLUME_CORRUPTED";
        case EFI_VOLUME_FULL:
            return L"EFI_VOLUME_FULL";
        case EFI_NO_MEDIA:
            return L"EFI_NO_MEDIA";
        case EFI_MEDIA_CHANGED:
            return L"EFI_MEDIA_CHANGED";
        case EFI_NOT_FOUND:
            return L"EFI_NOT_FOUND";
        case EFI_ACCESS_DENIED:
            return L"EFI_ACCESS_DENIED";
        case EFI_NO_RESPONSE:
            return L"EFI_NO_RESPONSE";
        case EFI_NO_MAPPING:
            return L"EFI_NO_MAPPING";
        case EFI_TIMEOUT:
            return L"EFI_TIMEOUT";
        case EFI_NOT_STARTED:
            return L"EFI_NOT_STARTED";
        case EFI_ALREADY_STARTED:
            return L"EFI_ALREADY_STARTED";
        case EFI_ABORTED:
            return L"EFI_ABORTED";
        case EFI_ICMP_ERROR:
            return L"EFI_ICMP_ERROR";
        case EFI_TFTP_ERROR:
            return L"EFI_TFTP_ERROR";
        case EFI_PROTOCOL_ERROR:
            return L"EFI_PROTOCOL_ERROR";
        default:
            return L"Unknown EFI_STATUS";
    }
}

// for testing
unsigned long read_cr2(void) {
    unsigned long cr2;
    __asm__ volatile ("mov %%cr2, %0" : "=r" (cr2));
    return cr2;
}

EFI_FILE_PROTOCOL* LoadFile(CHAR16* path, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    EFI_FILE_PROTOCOL* loadedFile;
    EFI_FILE_PROTOCOL* directory = NULL;

    EFI_LOADED_IMAGE_PROTOCOL* loadedImage;

    uefi_call_wrapper(SystemTable->BootServices->HandleProtocol, 3, ImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&loadedImage);


    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fileSystem;
    uefi_call_wrapper(SystemTable->BootServices->HandleProtocol, 3, loadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void**)&fileSystem);

    // unsigned long cr2 = read_cr2();
    // Print(L"cr2:0X%lx\n", cr2);

    uefi_call_wrapper(fileSystem->OpenVolume, 2, fileSystem, &directory);   // causes X64 exception type - 0D (general prot)

    EFI_STATUS s = uefi_call_wrapper(directory->Open, 5, directory, &loadedFile, path, EFI_FILE_MODE_READ, 0);

    // Print(L"%s\n", EfiStatusToStr(s));

    if (s != EFI_SUCCESS) {
        return NULL;
    }

    return loadedFile;
}

int eHeaderFormatCheck(Elf64_Ehdr* ehdr) {
    int statusCode = 0;

    //TODO add format check
}

EFI_STATUS efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
	EFI_STATUS Status;
	EFI_INPUT_KEY Key;

	ST = SystemTable; //Store the system table

	InitializeLib(ImageHandle, SystemTable);

    EFI_FILE_PROTOCOL* kernel = LoadFile(L"\\efi\\boot\\kernel.elf", ImageHandle, SystemTable); 
    if (kernel == NULL) {
        Print(L"kernel did not load\n");
    } 
    else {
        Print(L"kernel loaded successfully\n");
    }

    // get kernel.elf header
    Elf64_Ehdr header;
    {
        UINTN kernelInfoSize;
        EFI_FILE_INFO* kernelInfo;
        EFI_STATUS s;

        uefi_call_wrapper(kernel->GetInfo, 4, kernel, &gEfiFileInfoGuid, &kernelInfoSize, NULL);     // get size of kernel info
        
        uefi_call_wrapper(SystemTable->BootServices->AllocatePool, 3, EfiLoaderData, kernelInfoSize, (void**) &kernelInfo);    // alloc size for kernel GetInfo
        
        uefi_call_wrapper(kernel->GetInfo, 4, kernel, &gEfiFileInfoGuid, &kernelInfoSize, (void*) kernelInfo);    // store kernel info in allocated memory

        UINTN bufferSize = sizeof(header);
        s = uefi_call_wrapper(kernel->Read, 3, kernel, &bufferSize, (void*) &header);     // store pointer to kernel.elf eheader inside header var
        Print(L"%s\n", EfiStatusToStr(s));
        //TODO add EFI_BUFFER_TOO_SMALL check
        
        uefi_call_wrapper(SystemTable->BootServices->FreePool, 1, kernelInfo);    // return pool mem to system (necessary?)
    }

    //TODO add elf format check
    if (header.e_machine != EM_X86_64) {
        Print(L"kernel format incorrect\n");
    } 
    else {
        Print(L"kernel format correct\n");
    }
    
    // get kernel.elf program header
    Elf64_Phdr* pheaders;
    {
        EFI_STATUS s;
        UINTN pheadersSize = header.e_phentsize * header.e_phnum;

        Print(L"pheadersSize: %lu\n", pheadersSize);

        s = uefi_call_wrapper(kernel->SetPosition, 2, kernel, header.e_phoff);  // set file's current position to the pheader offset
        Print(L"setpos: %s\n", EfiStatusToStr(s));

        s = uefi_call_wrapper(SystemTable->BootServices->AllocatePool, 3, EfiLoaderData, pheadersSize, (void**) &pheaders); // alloc size for pheaders
        Print(L"palloc: %s\n", EfiStatusToStr(s));

        s = uefi_call_wrapper(kernel->Read, 3, kernel, &pheadersSize, (void*) pheaders);  // read pheaders from kernel into allocated memory
        Print(L"readpheader: %s\n", EfiStatusToStr(s));
    }
    
    EFI_MEMORY_DESCRIPTOR* mem_map;
    UINTN memmapsz;


    Elf64_Addr segmentVAddr;
    for (UINTN n = 0; n < header.e_phnum; n++) {
        Elf64_Phdr* pheader = (Elf64_Phdr*)((char*)pheaders + (n*header.e_phentsize));
        
        if (pheader->p_type == PT_LOAD) {
            UINTN pageCount = (pheader->p_memsz + PAGE_SIZE - 1) / PAGE_SIZE;
            // Elf64_Addr segmentVAddr; // = pheader->p_paddr;
            Print(L"seg: %d\n", segmentVAddr);
            EFI_STATUS s = uefi_call_wrapper(SystemTable->BootServices->AllocatePages, 4, AllocateAnyPages, EfiLoaderData, pageCount, &segmentVAddr);
            Print(L"allocpages: %s\n", EfiStatusToStr(s));
            
            s = uefi_call_wrapper(kernel->SetPosition, 2, kernel, pheader->p_offset);
            s = uefi_call_wrapper(kernel->Read, 3, kernel, (UINTN) &pheader->p_filesz, (void*) segmentVAddr);
            Print(L"read to page:%s\n", EfiStatusToStr(s));
        }
    }
    
    int (*kernelStart)() = ((__attribute__((sysv_abi)) int (*)() ) segmentVAddr);

    Print(L"kernel started with: %d\n", kernelStart());
    //TODO allocate memory for kernel
    //TODO load ELF segments
    //TODO handle segment attributes
    //TODO set up paging ?!
    //TODO set up CPU state
    //TODO transfer control to kernel

	return Status; // Exit the UEFI application
}
