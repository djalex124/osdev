#include <output/kterm.h>

#include <kernel/kstring.h>
#include <kernel/kernel.h>
#include <kernel/crash.h>
#include <kernel/debug.h>

#include <stddef.h>
#include <stdint.h>

uint64_t *kmem_bitmap_low;
size_t kmem_bitmap_low_max = 0;

uint64_t *kmem_bitmap_hi;
size_t kmem_bitmap_hi_max = 0;

void kmem_printpmminfo()
{
	size_t used_pages = 0;
	for (int i = 0; i < kmem_bitmap_low_max; i++)
	{
		if (kmem_bitmap_low[i] == UINT64_MAX - 1)
			used_pages += 64;
		else if (kmem_bitmap_low[i] == 0)
			continue;
		else
		{
			uint64_t count = kmem_bitmap_low[i];
			while (count > 0)
			{
				if (count & 1)
					used_pages++;
				count >>= 1;
			}
		}
	}
	for (int i = 0; i < kmem_bitmap_hi_max; i++)
	{
		if (kmem_bitmap_hi[i] == UINT64_MAX - 1)
			used_pages += 64;
		else if (kmem_bitmap_hi[i] == 0)
			continue;
		else
		{
			uint64_t count = kmem_bitmap_hi[i];
			while (count > 0)
			{
				if (count & 1)
					used_pages++;
				count >>= 1;
			}
		}
	}

	kterm_putf("\npmm stats: %d/%d (%d%%) frames used", used_pages,
		kmem_bitmap_low_max * 64 + kmem_bitmap_hi_max * 64,
		used_pages / (kmem_bitmap_low_max * 64 + kmem_bitmap_hi_max * 64));
}

void kmem_pmapset(uint64_t page, uint64_t length, uint8_t used, uint8_t low)
{
	uint64_t *bitmap = 0;
	uint64_t index = page;
	if (low == 0)
		bitmap = kmem_bitmap_low;
	else
	{
		bitmap = kmem_bitmap_hi;
		index -= 0x100000000 / 0x1000;
	}

	for (size_t b = 0; b < length; b++)
	{
		if (used)
			bitmap[(index + b) / 64] |= ((uint64_t)1 << ((index + b) % 64));
		else
			bitmap[(index + b) / 64] &= ~((uint64_t)1 << ((index + b) % 64));
	}
}

void kmem_pmminit(uint64_t low_size, uint64_t high_size)
{
	k_boottable.safe_mem = (k_boottable.safe_mem + 0xFFF) & ~0xFFF;

    uint64_t bitmap_pages_low = ((low_size / 8) + 0xFFF) & ~0xFFF;

    kmem_bitmap_low = (uint64_t *)k_boottable.safe_mem;
    kmem_bitmap_low_max = low_size / 64;
    k_boottable.safe_mem += bitmap_pages_low;
	memset((void *)kmem_bitmap_low, 0xFF, bitmap_pages_low);

#ifdef AQUA_DEBUG_MEM
	kdebug_outf("\nkm_ip: low bitmap @ 0x%x max entry 0x%x", kmem_bitmap_low, kmem_bitmap_low_max);
#endif

    if (high_size)
    {
        uint64_t bitmap_pages_hi = (((high_size - 0x100000000 / 0x1000) / 8) + 0xFFF) & ~0xFFF;

        kmem_bitmap_hi = (uint64_t *)k_boottable.safe_mem;
        kmem_bitmap_hi_max = (high_size - 0x100000000 / 0x1000) / 64;
        k_boottable.safe_mem += bitmap_pages_hi;
		memset((void *)kmem_bitmap_hi, 0xFF, bitmap_pages_hi);
#ifdef AQUA_DEBUG_MEM
		kdebug_outf("\nkm_ip: high bitmap @ 0x%x max entry 0x%x", kmem_bitmap_hi, kmem_bitmap_hi_max);
#endif
    }
    
    efi_memory_descriptor *mmap = k_boottable.mmap;
    efi_memory_descriptor *mmap_entries;
    uint64_t mmap_length = k_boottable.mmap_enteries * k_boottable.mmap_size;

    for (mmap_entries = mmap;
        (uint8_t *)mmap_entries < (uint8_t *)mmap + mmap_length;
        mmap_entries = (efi_memory_descriptor *) ((uint64_t)mmap_entries + k_boottable.mmap_size))
    {
		switch (mmap_entries->type)
		{
			case 1 ... 4: //os/uefi loader data
			case 7:   //conventional free
				if ((mmap_entries->physical_start >= 0x100000000) && high_size)
					kmem_pmapset(mmap_entries->physical_start / 0x1000, mmap_entries->num_pages, 0, 1);
				else if (mmap_entries->physical_start < 0x100000000)
					kmem_pmapset(mmap_entries->physical_start / 0x1000, mmap_entries->num_pages, 0, 0);
				break;
		}
    }

	uint64_t kernel_length = phys_from_virt(k_boottable.safe_mem) / 0x1000;
	kmem_pmapset(0, kernel_length, 1, 0);
}

//eventually, add distinctions for additional requirements (isa, 32-bit, etc)
//should only be called by kmem_alloc
void* kmem_palloc(size_t pages)
{
	int64_t continuous_start = -1;
	uint64_t continuous_open = 0;
	for (size_t i = 0; i < kmem_bitmap_low_max; i++)
	{
		if (kmem_bitmap_low[i] == UINT64_MAX - 1)
		{
			continuous_start = -1;
			continuous_open = 0;
			continue;
		}

		for (int b = 0; b < 64; b++)
		{
			if (continuous_open == pages)
				break;

			if (!(kmem_bitmap_low[i] & ((uint64_t)1 << b)))
			{
				if (continuous_start == -1)
					continuous_start = i * 64 + b;
				continuous_open++;
			}
			else
			{
				continuous_start = -1;
				continuous_open = 0;
			}
		}

		if (continuous_open == pages)
			break;
	}

	if (kmem_bitmap_hi_max && (continuous_open != pages))
	{
		for (size_t i = 0; i < kmem_bitmap_hi_max; i++)
		{
			if (kmem_bitmap_hi[i] == UINT64_MAX - 1)
			{
				continuous_start = -1;
				continuous_open = 0;
				continue;
			}

			for (int b = 0; b < 64; b++)
			{
				if (continuous_open == pages)
					break;

				if (!(kmem_bitmap_hi[i] & ((uint64_t)1 << b)))
				{
					if (continuous_start == -1)
						continuous_start = i * 64 + b;
					continuous_open++;
				}
				else
				{
					continuous_start = -1;
					continuous_open = 0;
				}
			}

			if (continuous_open == pages)
				break;
		}
	}

#ifdef AQUA_DEBUG_MEM
	kdebug_outf("\nkm_pa: found %d pages at 0x%x", pages, continuous_start * 0x1000);
#endif

	if (continuous_open == pages)
	{
		uint64_t phys_addr = continuous_start * 0x1000;
		if (phys_addr >= 0x100000000)
			kmem_pmapset(continuous_start, continuous_open, 1, 1);
		else
			kmem_pmapset(continuous_start, continuous_open, 1, 0);

		memset((void *)phys_addr, 0, pages * 0x1000);
		return (void *)phys_addr;
	}
	
	kdebug_outf("\nkm_p: unable to gather 0x%x pages, returning null", pages);
	return (void *)0;
}

//should only be called by kmem_free
void kmem_pfree(void* addr, size_t pages)
{
	uint64_t phys_addr = (uint64_t)addr;
	if (phys_addr >= 0x100000000)
	{
		if ((phys_addr / 0x1000) + pages >= kmem_bitmap_hi_max * 64)
			kcrash("km_p: free above range");

		kmem_pmapset(phys_addr / 0x1000, pages, 0, 1);
	}
	else
	{
		if ((phys_addr / 0x1000) + pages >= kmem_bitmap_low_max * 64)
			kcrash("km_p: free above range");

		kmem_pmapset(phys_addr / 0x1000, pages, 0, 0);
	}
}