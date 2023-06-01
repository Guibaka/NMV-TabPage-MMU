#include <memory.h>
#include <printk.h>
#include <string.h>
#include <x86.h>
#include <pgt.h>

#define PHYSICAL_POOL_PAGES  64
#define PHYSICAL_POOL_BYTES  (PHYSICAL_POOL_PAGES << 12)
#define BITSET_SIZE          (PHYSICAL_POOL_PAGES >> 6)


extern __attribute__((noreturn)) void die(void);

static uint64_t bitset[BITSET_SIZE];

static uint8_t pool[PHYSICAL_POOL_BYTES] __attribute__((aligned(0x1000)));


paddr_t alloc_page(void)
{
	size_t i, j;
	uint64_t v;

	for (i = 0; i < BITSET_SIZE; i++) {
		if (bitset[i] == 0xffffffffffffffff)
			continue;

		for (j = 0; j < 64; j++) {
			v = 1ul << j;
			if (bitset[i] & v)
				continue;

			bitset[i] |= v;
			return (((64 * i) + j) << 12) + ((paddr_t) &pool);
		}
	}

	printk("[error] Not enough identity free page\n");
	return 0;
}

void free_page(paddr_t addr)
{
	paddr_t tmp = addr;
	size_t i, j;
	uint64_t v;

	tmp = tmp - ((paddr_t) &pool);
	tmp = tmp >> 12;

	i = tmp / 64;
	j = tmp % 64;
	v = 1ul << j;

	if ((bitset[i] & v) == 0) {
		printk("[error] Invalid page free %p\n", addr);
		die();
	}

	bitset[i] &= ~v;
}


/*
 * Memory model for Rackdoll OS
 *
 * +----------------------+ 0xffffffffffffffff
 * | Higher half          |
 * | (unused)             |
 * +----------------------+ 0xffff800000000000
 * | (impossible address) |
 * +----------------------+ 0x00007fffffffffff
 * | User                 |
 * | (text + data + heap) |
 * +----------------------+ 0x2000000000
 * | User                 |
 * | (stack)              |
 * +----------------------+ 0x40000000
 * | Kernel               |
 * | (valloc)             |
 * +----------------------+ 0x201000
 * | Kernel               |
 * | (APIC)               |
 * +----------------------+ 0x200000
 * | Kernel               |
 * | (text + data)        |
 * +----------------------+ 0x100000
 * | Kernel               |
 * | (BIOS + VGA)         |
 * +----------------------+ 0x0
 *
 * This is the memory model for Rackdoll OS: the kernel is located in low
 * addresses. The first 2 MiB are identity mapped and not cached.
 * Between 2 MiB and 1 GiB, there are kernel addresses which are not mapped
 * with an identity table.
 * Between 1 GiB and 128 GiB is the stack addresses for user processes growing
 * down from 128 GiB.
 * The user processes expect these addresses are always available and that
 * there is no need to map them explicitely.
 * Between 128 GiB and 128 TiB is the heap addresses for user processes.
 * The user processes have to explicitely map them in order to use them.
 */


void map_page(struct task *ctx, vaddr_t vaddr, paddr_t paddr)
{
	uint64_t *p = (uint64_t *) ctx->pgt;
	paddr_t tmp;

	printk("\nPLM4 %p, PLM3 %p, PLM2 %p, PLM1 %p\n", 
		PGT_PLM4_INDEX(vaddr),PGT_PLM3_INDEX(vaddr), 
		PGT_PLM2_INDEX(vaddr), PGT_PLM1_INDEX(vaddr));

	// PML4
	if(!PGT_IS_VALID(PGT_PLM4_INDEX(vaddr)))
	{
	  printk("Entry is invalid, allocate PLM4\n");

	  tmp = alloc_page();

	  memset((void *) tmp, 0, PGT_SIZE);
	  p[PGT_PLM4_INDEX(vaddr)] |= (tmp | PGT_VALID_MASK | PGT_WRITABLE_MASK | PGT_USER_MASK);
	}else{
		printk("Valid page tab\n");
	}

	p = (uint64_t *) PGT_ADDRESS(p[PGT_PLM4_INDEX(vaddr)]);
	printk("%p\n", p);

	// PML3
	if(!PGT_IS_VALID(PGT_PLM3_INDEX(vaddr)))
	{
	  printk("Entry is invalid, allocate PLM3\n");

	  tmp = alloc_page();

	  memset((void *) tmp, 0, PGT_SIZE);
	  p[PGT_PLM3_INDEX(vaddr)] |= (tmp | PGT_VALID_MASK | PGT_WRITABLE_MASK | PGT_USER_MASK);
	}else{
		printk("Valid page tab\n");
	}

	p = (uint64_t *) PGT_ADDRESS(p[PGT_PLM3_INDEX(vaddr)]);
	printk("%p\n", p);

	// PML2
	if(!PGT_IS_VALID(PGT_PLM2_INDEX(vaddr)))
	{
	  printk("Entry is invalid, allocate PLM2\n");

	  tmp = alloc_page();

	  memset((void *) tmp, 0, PGT_SIZE);
	  p[PGT_PLM2_INDEX(vaddr)] |= (tmp | PGT_VALID_MASK | PGT_WRITABLE_MASK | PGT_USER_MASK);
	}else{
		printk("Valid page tab\n");
	}

	p = (uint64_t *) PGT_ADDRESS(p[PGT_PLM2_INDEX(vaddr)]);
	printk("%p\n", p);

	// PML1
	if(!PGT_IS_VALID(PGT_PLM1_INDEX(vaddr)))
	{
	  printk("Entry is invalid, allocate PLM1\n");

	  memset((void *) paddr, 0, PGT_SIZE);
	  p[PGT_PLM1_INDEX(vaddr)] |= (paddr | PGT_VALID_MASK | PGT_WRITABLE_MASK | PGT_USER_MASK);
	}else{
		printk("Virtual address already mapped\n");
	}
}

void load_task(struct task *ctx)
{
	uint64_t *p;
	paddr_t paddr;
	vaddr_t vaddr;

	// Alloc new page for new process
	paddr = alloc_page();
	memset((void *) paddr, 0, PGT_SIZE);
	ctx->pgt = paddr;

	// Get current process page 
	p = (uint64_t *) ctx->pgt;
	paddr = alloc_page(); // Alloc PLM4
	memset((void *)paddr, 0, PGT_SIZE);
	p[0] |= (paddr | PGT_VALID_MASK | PGT_WRITABLE_MASK | PGT_USER_MASK);

	// Get address of TLB 
	p = (uint64_t *) store_cr3(); //PLM4
	paddr = PGT_ADDRESS(*(uint64_t *)PGT_ADDRESS(*p));  //PLM3 where kernel code is stored
	p = (uint64_t *) PGT_ADDRESS(*((uint64_t *)ctx->pgt));
	p[0] |= (paddr | PGT_VALID_MASK | PGT_WRITABLE_MASK | PGT_USER_MASK);

	// process payload initalization
	for(paddr = ctx->load_paddr; paddr < ctx->load_end_paddr; paddr+=PGT_SIZE)
	{
		vaddr = ctx->load_vaddr + paddr - ctx->load_paddr;
		map_page(ctx, vaddr, paddr);
	}

	// process bss initialization
	for(vaddr = ctx->load_vaddr + ctx->load_end_paddr - ctx->load_paddr; 
		vaddr < ctx->bss_end_vaddr; vaddr += PGT_SIZE)
	{
		paddr = alloc_page();
		memset((void *) paddr, 0, PGT_SIZE);
		map_page(ctx, vaddr, paddr);
	}

}

void set_task(struct task *ctx)
{
	load_cr3(ctx->pgt); 
}

void pgfault(struct interrupt_context *ctx)
{
	printk("Page fault at %p\n", ctx->rip);
	printk("  cr2 = %p\n", store_cr2());

	uint64_t * faddr = (uint64_t *) store_cr2();

	if(USER_STACK_BEGIN <= *((uint64_t *) PGT_ADDRESS(*faddr)) && *((uint64_t *) PGT_ADDRESS(*faddr)) < USER_STACK_END){
		paddr_t paddr;
		paddr = alloc_page();
		struct task task_ctx;
		task_ctx.pgt = (paddr_t) *((uint64_t *) PGT_ADDRESS(*faddr)); 
		map_page(&task_ctx, (vaddr_t) *faddr, paddr);
	}else{
		exit_task(ctx);
	}

}

void mmap(struct task *ctx, vaddr_t vaddr)
{
	paddr_t paddr;
	paddr = alloc_page();
	memset((void *) paddr, 0, PGT_SIZE);
	map_page(ctx, vaddr, paddr);
}

void munmap(struct task *ctx, vaddr_t vaddr)
{
	uint64_t *p = (uint64_t *) ctx->pgt;

	uint64_t *paddr4 = (uint64_t *) p[PGT_PLM4_INDEX(vaddr)];
	uint64_t *paddr3 = (uint64_t *) p[PGT_PLM3_INDEX(vaddr)];
	uint64_t *paddr2 = (uint64_t *) p[PGT_PLM2_INDEX(vaddr)];
	
	free_page(p[PGT_PLM1_INDEX(vaddr)]);
	free_page((paddr_t) *paddr2);
	free_page((paddr_t) *paddr3);
	free_page((paddr_t) *paddr4);
}

void duplicate_task(struct task *ctx)
{
}
