#include <efi.h>
#include <efilib.h>
#include <elf.h>

// used to debug the 0x800000000000000E error code received from EFI_FILE_PROTOCOL->Open
const CHAR16* EfiStatusToStr(EFI_STATUS s) {
    switch (s) {
        case EFI_SUCCESS:
            return L"EFI_SUCCESS";
        case EFI_NOT_FOUND:
            return L"EFI_NOT_FOUND";
        case EFI_NO_MEDIA:
            return L"EFI_NO_MEDIA";
        case EFI_MEDIA_CHANGED:
            return L"EFI_MEDIA_CHANGED";
        case EFI_DEVICE_ERROR:
            return L"EFI_DEVICE_ERROR";
        case EFI_VOLUME_CORRUPTED:
            return L"EFI_VOLUME_CORRUPTED";
        case EFI_WRITE_PROTECTED:
            return L"EFI_WRITE_PROTECTED";
        case EFI_ACCESS_DENIED:
            return L"EFI_ACCESS_DENIED";
        case EFI_OUT_OF_RESOURCES:
            return L"EFI_OUT_OF_RESOURCES";
        case EFI_VOLUME_FULL:
            return L"EFI_VOLUME_FULL";
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

EFI_STATUS efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
	EFI_STATUS Status;
	EFI_INPUT_KEY Key;

	ST = SystemTable; //Store the system table

	InitializeLib(ImageHandle, SystemTable);

    EFI_FILE_PROTOCOL* kernel = LoadFile(L"\\efi\\boot\\kernel.elf", ImageHandle, SystemTable); 
    if (kernel == NULL) {
        Print(L"kernel did not load");
    } 
    else {
        Print(L"kernel loaded successfully");
    }

    Elf64_Ehdr header;
    {
        UINTN kernelInfoSize;
        EFI_FILE_INFO* kernelInfo;

        uefi_call_wrapper(kernel->GetInfo, 4, kernel, &gEfiFileInfoGuid, &kernelInfoSize, NULL);     // get size of kernel info
        uefi_call_wrapper(SystemTable->BootServices->AllocatePool, 3, EfiLoaderData, kernelInfoSize, (void**) &kernelInfo);    // alloc size for kernel info
        uefi_call_wrapper(kernel->GetInfo, 4, kernel, &gEfiFileInfoGuid, &kernelInfoSize, (void**) &kernelInfo);    // store kernel info in allocated memory
        

        uefi_call_wrapper(kernel->Read, 3, kernel, (UINTN*) sizeof(header), &header);     // store kernel.elf eheader inside the header var
        //TODO add EFI_BUFFER_TOO_SMALL check
        
        uefi_call_wrapper(SystemTable->BootServices->FreePool, 1, kernelInfo);    // return pool mem to system (necessary?)
    }

    //TODO add check for e_machine?
    if (header.e_machine != EM_X86_64) {
        Print(L"kernel format incorrect");
    } 
    else {
        Print(L"kernel format correct");
    }

    //TODO allocate memory for kernel
    //TODO load ELF segments
    //TODO handle segment attributes
    //TODO set up paging ?
    //TODO set up CPU state
    //TODO transfer control to kernel

	return Status; // Exit the UEFI application
}
