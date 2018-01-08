#include <stddef.h>
#include <elf.h>

#include <ps4/kernel.h>
#include <ps4/kernel/firmware_tables.h>

extern uint32_t sdkVersion;

static Elf64_Sym *ps4KernelDlSymElfSymbols;
static char *ps4KernelDlSymElfStrings;

static void ps4KernelDlSymInitialize(void)
{
	void *kernelAddress = ps4KernelSeekElfAddress();
	if(kernelAddress == NULL)
		return;
		

	// Ah, old-school/kernel C naming conventions ... terrible stuff
	Elf64_Ehdr *ehdr = (Elf64_Ehdr *)kernelAddress;
	/*for(size_t i = 0; i < ehdr->e_phnum; i++)
	{
		Elf64_Phdr *phdr = (Elf64_Phdr *)(kernelAddress + ehdr->e_phoff) + i;
		if (phdr->p_type == PT_PHDR)
			ehdr = (Elf64_Ehdr *)(phdr->p_vaddr - ehdr->e_phoff);
	}*/
	Elf64_Phdr *phdr = (Elf64_Phdr *)((uint8_t *)ehdr + ehdr->e_phoff);
	for(size_t i = 0; i < ehdr->e_phnum; i++, phdr++)
		if (phdr->p_type == PT_DYNAMIC)
		{
			Elf64_Dyn *dyn = (Elf64_Dyn *)phdr->p_vaddr;
			for (Elf64_Dyn *d = dyn; d->d_tag != DT_NULL; d++)
				switch (d->d_tag)
				{
					case DT_SYMTAB:
						ps4KernelDlSymElfSymbols = (Elf64_Sym *)d->d_un.d_ptr;
						break;
					case DT_STRTAB:
						ps4KernelDlSymElfStrings = (char *)d->d_un.d_ptr;
						break;
				}
		}
}

int ps4KernelStaticLookup(const char *name, void **value)
{
	void* kernel_base = ps4KernelSeekElfAddress();

	sym_t* table;
	switch(sdkVersion)
	{
		case 0x04050001:
			table = table405;
			break;
		default:
			return PS4_ERROR_KERNEL_SYMBOL_LOOKUP_NOT_FOUND;
	}

	for (sym_t *p = table; p->name != NULL; ++p)
	{
		const char *n = p->name;
	
		for(int j = 0; n[j] == name[j]; ++j)
		{
			if(j > 0 && n[j] == '\0' && name[j] == '\0')
			{
					*(uint64_t *)value = (uint64_t *)kernel_base + p->offset;
					return PS4_OK;
			}
			else if(n[j] == '\0' || name[j] == '\0')
			{
				break;
			}
		}
	}
	return PS4_ERROR_KERNEL_SYMBOL_LOOKUP_NOT_FOUND;
}

void *ps4KernelDlSym(char *name)
{
	Elf64_Sym *symbol;
	if(name==NULL)
	{
		return NULL;
	}
	if(sdkVersion > 0x01760001)
	{
		void **address = NULL;
		ps4KernelStaticLookup(name, address);
		return address;
	}
	if(ps4KernelDlSymElfSymbols == NULL || ps4KernelDlSymElfStrings == NULL)
		ps4KernelDlSymInitialize();

	for(symbol = ps4KernelDlSymElfSymbols; symbol + 1 < (Elf64_Sym *)ps4KernelDlSymElfStrings; ++symbol)
	{
		int j;
		char *n = (char *)&ps4KernelDlSymElfStrings[symbol->st_name];
		for(j = 0; n[j] == name[j] && n[j] != 0; ++j);
		if(j > 0 && n[j] == '\0' && name[j] == '\0')
			return (void *)symbol->st_value;
	}

	return NULL;
}
