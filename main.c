#include <efi.h>
#include <efilib.h>
#include <elf.h>

EFI_FILE_PROTOCOL* LoadFile(CHAR16* path, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    EFI_FILE_PROTOCOL* loadedFile;
    EFI_FILE_PROTOCOL* directory = NULL;

    EFI_LOADED_IMAGE_PROTOCOL* loadedImage;
    SystemTable->BootServices->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&loadedImage);

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fileSystem;
    SystemTable->BootServices->HandleProtocol(loadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void**)&fileSystem);

    EFI_STATUS openVolumeStatus = fileSystem->OpenVolume(fileSystem, &directory);   // causes X64 exception type - 0D (general prot)

    EFI_STATUS s = directory->Open(directory, &loadedFile, path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);

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

    EFI_FILE_PROTOCOL* kernel = LoadFile(L"kernel.elf", ImageHandle, SystemTable); 
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

        kernel->GetInfo(kernel, &gEfiFileInfoGuid, &kernelInfoSize, NULL);     // get size of kernel info
        SystemTable->BootServices->AllocatePool(EfiLoaderData, kernelInfoSize, (void**) &kernelInfo);    // alloc size for kernel info
        kernel->GetInfo(kernel, &gEfiFileInfoGuid, &kernelInfoSize, (void**) &kernelInfo);    // store kernel info in allocated memory
        
        kernel->Read(kernel, (UINTN*) sizeof(header), &header);     // store kernel.elf eheader inside the header var
        //TODO add EFI_BUFFER_TOO_SMALL check

        SystemTable->BootServices->FreePool(kernelInfo);    // return pool mem to system (necessary?)
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
