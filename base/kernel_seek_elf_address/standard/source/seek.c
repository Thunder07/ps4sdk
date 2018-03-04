#include <stddef.h>
#include <sys/types.h>
#define PS4_PAGE_SIZE 0x4000
#define PS4_KERNEL_ELF_RANGE_ADDRESS (char *)0xffffffff80000000
enum{ PS4_KERNEL_ELF_RANGE_SIZE = 0x02000000 };
#define PS4_KERNEL_ELF_FIXED_ADDRESS (char *)0xffffffff80700000
//enum{ PS4_KERNELEL_ELF_SIZE = 0x00EAC180 };
enum{ PS4_KERNEL_ELF_PAGE_SIZE = 16 * 1024 }; // no dependencies

extern uint32_t sdkVersion;

void *ps4KernelFindSkippedPageStart(uint64_t addr, size_t skip)
{
	int remainder = (addr + (skip * PS4_PAGE_SIZE)) % PS4_PAGE_SIZE;
	if(remainder == 0)
		return (void*)addr;
	else
		return (void*)(addr + PS4_PAGE_SIZE - remainder);
}

void *ps4KernelFindClosestPageStart(uint64_t addr)
{
	return ps4KernelFindSkippedPageStart(addr, 0);
}

void *ps4KernelSeekElfAddress()
{
	static char *addr = NULL;
	if(addr != NULL) return addr;
	
	char *m;
	int i;
	unsigned char elfMagic[] = {0x7f, 'E', 'L', 'F', 0x02, 0x01, 0x01, 0x09, 0x00};
	const size_t magicSize = sizeof(elfMagic) / sizeof(*elfMagic);
	
	if(sdkVersion>0x01760001)
	{
		uint64_t rip;
		{
			unsigned long __edx;
			unsigned long __eax;
			__asm__ ("rdmsr" : "=d"(__edx), "=a"(__eax) : "c"(0xC0000082));
			rip = (((uint64_t)__edx) << 32) | (uint64_t)__eax;
		}
	
		char *startaddr = ps4KernelFindClosestPageStart(rip);
		if(sdkVersion == 0x4050001)
			startaddr = ps4KernelFindSkippedPageStart(rip, -196);
	
		for(char *m = startaddr;m > startaddr - PS4_KERNEL_ELF_RANGE_SIZE;m -= PS4_KERNEL_ELF_PAGE_SIZE)
		{
			for(i = 0; i < magicSize && m[i] == elfMagic[i]; ++i);
			if(i == magicSize)
			{
				addr = m;
				return m;
			}
		}
		return NULL;
	}
	else
	{
		m = PS4_KERNEL_ELF_FIXED_ADDRESS;
		for(i = 0; i < magicSize && m[i] == elfMagic[i]; ++i);
			if(i == magicSize)
			{
				addr = m;
				return m;
			}
			for(m = PS4_KERNEL_ELF_RANGE_ADDRESS;
				m < PS4_KERNEL_ELF_RANGE_ADDRESS + PS4_KERNEL_ELF_RANGE_SIZE;
				m += PS4_KERNEL_ELF_PAGE_SIZE)
			{
				for(i = 0; i < magicSize && m[i] == elfMagic[i]; ++i);
				if(i == magicSize)
				{
					addr = m;
					return m;
				}
			}

		return NULL;
	}
}
