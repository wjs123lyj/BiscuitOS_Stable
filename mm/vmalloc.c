#include "linux/kernel.h"
#include "linux/pgtable.h"
#include "linux/rbtree.h"
#include "linux/vmalloc.h"
#include "linux/bitops.h"
#include "linux/gfp.h"
#include "asm/errno-base.h"
#include "linux/rculist.h"
#include "linux/cpumask.h"
#include "linux/spinlock_types.h"
#include "linux/radix-tree.h"
#include "linux/atomic.h"
#include "linux/mm.h"
#include "linux/debug.h"
#include "linux/kmemleak.h"
#include "linux/hardirq.h"
#include "linux/debug_locks.h"
#include "linux/cacheflush.h"
#include "linux/rwlock.h"
#include "linux/pgtable-nopud.h"
#include "linux/spinlock.h"
#include "linux/err.h"
#include "linux/slab.h"
#include "linux/printk.h"
#include "linux/bitmap.h"
#include "linux/rcutiny_plugin.h"
#include "linux/tlbflush.h"
#include "linux/mutex.h"
#include "linux/threads.h"

/**** Global kva allocator ****/

#define VM_LAZY_FREE         0x01
#define VM_LAZY_FREEING      0x02
#define VM_VM_AREA           0x04
/*
 * If we had a constant VMALLOC_START and VMALLOC_END,we'd like to be able
 * to #define VMALLOC_SPACE  (VMALLOC_END - VMALLOC_START).Guess
 * instead.
 */
#define VMALLOC_SPACE  (128UL * 1024 * 1024)

#define VMALLOC_PAGES  (VMALLOC_SPACE / PAGE_SIZE)
#define VMAP_MAX_ALLOC BITS_PER_LONG  /* 256K with 4K pages */
#define VMAP_BBMAP_BITS_MAX   1024 /* 4MB with 4K pages */
#define VMAP_BBMAP_BITS_MIN   (VMAP_MAX_ALLOC * 2)
#define VMAP_MIN(x,y)    ((x) < (y) ? (x) : (y)) /* can't use min() */
#define VMAP_MAX(x,y)    ((x) > (y) ? (x) : (y)) /* can't use max() */
#define VMAP_BBMAP_BITS  VMAP_MIN(VMAP_BBMAP_BITS_MAX,      \
		                 VMAP_MAX(VMAP_BBMAP_BITS_MIN,      \
							VMALLOC_PAGES / NR_CPUS / 16))
#define VMAP_BLOCK_SIZE  (VMAP_BBMAP_BITS * PAGE_SIZE)



struct vmap_block_queue {
	spinlock_t lock;
	struct list_head free;
};

struct vmap_block {
	spinlock_t lock;
	struct vmap_area *va;
	struct vmap_block_queue *vbq;
	unsigned long free,dirty;
	DECLARE_BITMAP(alloc_map,VMAP_BBMAP_BITS);
	DECLARE_BITMAP(dirty_map,VMAP_BBMAP_BITS);
	struct list_head free_list;
	struct rcu_head  rcu_head;
	struct list_head purge;
};

/* Queue of free and dirty vmap blocks,for allocation and flushing purposes */
static DEFINE_PER_CPU(struct vmap_block_queue,vmap_block_queue);

int vmlist_lock;
//DEFINE_RWLOCK(vmlist_lock);
struct vm_struct *vmlist;

int vmap_area_lock;
//DEFINE_SPINLOCK(vmap_area_lock);
static struct rb_root vmap_area_root = RB_ROOT;
unsigned long vmap_area_pcpu_hole;
static atomic_t vmap_lazy_nr = ATOMIC_INIT(0);
static LIST_HEAD(vmap_area_list);
static bool vmap_initialized __read_mostly = false;
extern unsigned long totalram_pages;
extern void kfree(const void *x);
/*
 * Radix tree of vmap blocks,indexed by address,to quickly find a vmap block
 * in the free path.Could get rid of this if we change the API to return a
 * 'cookie' from alloc,to be passed to free.But no big deal yet.
 */
static int vmap_block_tree_lock;

//static DEFINE_SPINLOCK(vmap_block_tree_lock);
static RADIX_TREE(vmap_block_tree,GFP_ATOMIC);
/**** Per cpu kva allocator ****/

/*
 * vmap space is limited especially on 32 bit architectures,Ensure there is
 * room for at least 16 percpu vmap blocks per CPU.
 */


/*
 * We should probably have a fallback mechinism to allocate virtual memory
 * out of partially filled vmap blocks.However vmap block sizing should be
 * fairly reasonable according to the vmalloc size,so if shouldn't be a 
 * bit problem.
 */
static unsigned long addr_to_vb_idx(unsigned long addr)
{
	addr -= VMALLOC_START & ~(VMAP_BLOCK_SIZE - 1);
	addr /= VMAP_BLOCK_SIZE;
	return addr;
}
/*
 * Lazy_max_pages is the maximum amount of virtual address space we gather up
 * before attempting to purge with a TLB flush.
 */
static unsigned long lazy_max_pages(void)
{
	unsigned long log;

	log = fls(num_online_cpus());

	return log * (32UL * 1024 * 1024 / PAGE_SIZE);
}
static void rcu_free_va(struct rcu_head *head)
{
	struct vmap_area *va = container_of(head,struct vmap_area,rcu_head);

	kfree(va);
}

static void __free_vmap_area(struct vmap_area *va)
{
	BUG_ON(RB_EMPTY_NODE(&va->rb_node));
	rb_erase(&va->rb_node,&vmap_area_root);
	RB_CLEAR_NODE(&va->rb_node);
	list_del_rcu(&va->list);

	/*
	 * Track the highest possible candidatae for pcpu area
	 * allocation.Areas outside of vmalloc area can be returned
	 * here too,consider only end addresses which fall inside
	 * vmalloc area proper.
	 */
	if(va->va_end > VMALLOC_START && va->va_end <= VMALLOC_END)
		vmap_area_pcpu_hole = max(vmap_area_pcpu_hole,va->va_end);

	call_rcu(&va->rcu_head,rcu_free_va);
}
int is_vmalloc_or_module_addr(const void *x)
{
	/*
	 * ARM put module in a special place,
	 * and fall back on vmalloc() if that fails.Others
	 * just put in the vmalloc space.
	 */
#if defined(CONFIG_MODULES) && defined(MODULES_VADDR)
	unsigned long addr = (unsigned long)x;

	if(addr >= MODULES_VADDR && addr < MODULES_END)
		return 1;
#endif
	return is_vmalloc_addr(x);
}
static void purge_fragmented_blocks_allcpus(void);
/*
 * Purges all lazily - free vmap areas.
 */
static void __purge_vmap_area_lazy(unsigned long *start,unsigned long *end,
		int sync,int force_flush)
{
	static DEFINE_SPINLOCK(purge_lock);
	LIST_HEAD(valist);
	struct vmap_area *va;
	struct vmap_area *n_va;
	int nr = 0;

	/*
	 * If sync is 0 but force_flush is 1,we'll go sync anyway but callers
	 * should not expect such behavious.This just simplifies locking for
	 * the case that isn't actually used at the moment anyway.
	 */
	if(!sync && !force_flush)
	{
		if(!spin_trylock(&purge_lock))
			return;
	} else
		spin_lock(&purge_lock);
	if(sync)
		purge_fragmented_blocks_allcpus();

	rcu_read_lock();
	list_for_each_entry_rcu(va,&vmap_area_list,list)
	{
		if(va->flags & VM_LAZY_FREE)
		{
			if(va->va_start < *start)
				*start = va->va_start;
			if(va->va_end > *end)
				*end = va->va_end;
			nr += (va->va_end - va->va_start) >> PAGE_SHIFT;
			list_add_tail(&va->purge_list,&valist);
			va->flags |= VM_LAZY_FREEING;
			va->flags &= ~VM_LAZY_FREE;
		}
	}
	rcu_read_unlock();

	if(nr)
		atomic_sub(nr,&vmap_lazy_nr);

	if(nr || force_flush)
		flush_tlb_kernel_range(*start,*end);

	if(nr)
	{
		spin_lock(&vmap_area_lock);
		list_for_each_entry_safe(va,n_va,&valist,purge_list)
			__free_vmap_area(va);
		spin_unlock(&vmap_area_lock);
	}
	spin_unlock(&purge_lock);
}
/*
 * Kick off a purge of the outstanding lazy areas.Don't bother if somebody
 * is already purging.
 */
static void try_purge_vmap_area_lazy(void)
{
	unsigned long start = ULONG_MAX,end = 0;

	__purge_vmap_area_lazy(&start,&end,0,0);
}
static void free_vmap_area_noflush(struct vmap_area *va);
static void rcu_free_vb(struct rcu_head *head);
/*
 * 
 */
static void free_vmap_block(struct vmap_block *vb)
{
	struct vmap_block *tmp;
	unsigned long vb_idx;

	vb_idx = addr_to_vb_idx(vb->va->va_start);
	spin_lock(&vmap_block_tree_lock);
	tmp = (struct vmap_block *)(unsigned long)radix_tree_delete(
					&vmap_block_tree,vb_idx);
	spin_unlock(&vmap_block_tree_lock);
	BUG_ON(tmp != vb);

	free_vmap_area_noflush(vb->va);
	call_rcu(&vb->rcu_head,rcu_free_vb);
}
static void purge_fragmented_blocks(int cpu)
{
	LIST_HEAD(purge);
	struct vmap_block *vb;
	struct vmap_block *n_vb;
	struct vmap_block_queue *vbq;

	rcu_read_lock();
	list_for_each_entry_rcu(vb,&vbq->free,free_list)
	{
		if(!(vb->free + vb->dirty == VMAP_BBMAP_BITS && 
					vb->dirty != VMAP_BBMAP_BITS))
			continue;
		spin_lock(&vb->lock);
		if(vb->free + vb->dirty == VMAP_BBMAP_BITS &&
				vb->dirty != VMAP_BBMAP_BITS)
		{
			vb->free = 0; /* prevent further allocs after releasing lock */
			vb->dirty = VMAP_BBMAP_BITS; /* prevent purging it again */
			bitmap_fill((unsigned long *)(unsigned long)vb->alloc_map,
					VMAP_BBMAP_BITS);
			bitmap_fill((unsigned long *)(unsigned long)vb->dirty_map,
					VMAP_BBMAP_BITS);
			spin_lock(&vbq->lock);
			list_del_rcu(&vb->free_list);
			spin_unlock(&vbq->lock);
			spin_unlock(&vb->lock);
			list_add_tail(&vb->purge,&purge);
		} else
			spin_unlock(&vb->lock);
	}
	rcu_read_unlock();

	list_for_each_entry_safe(vb,n_vb,&purge,purge)
	{
		list_del(&vb->purge);
		free_vmap_block(vb);
	}
}

static void purge_fragmented_blocks_allcpus(void)
{
	int cpu;

	for_each_possible_cpu(cpu)
		purge_fragmented_blocks(cpu);
}
/*
 * Kick off a purge of the outstanding lazy areas.
 */
static void purge_vmap_aera_lazy(void)
{
	unsigned long start = ULONG_MAX,end = 0;

	__purge_vmap_area_lazy(&start,&end,1,0);
}

void __insert_vmap_area(struct vmap_area *va)
{
	struct rb_node **p = &vmap_area_root.rb_node;
	struct rb_node *parent = NULL;
	struct rb_node *tmp;

	while(*p) {
		struct vmap_area *tmp_va;

		parent = *p;
		tmp_va = rb_entry(parent,struct vmap_area,rb_node);
		if(va->va_start < tmp_va->va_end)
			p = &(*p)->rb_left;
		else if(va->va_end > tmp_va->va_start)
			p = &(*p)->rb_right;
		else
			BUG();
	}
	rb_link_node(&va->rb_node,parent,p);
	rb_insert_color(&va->rb_node,&vmap_area_root);

	/* address-sort this list so it is usable like the vmlist */
	tmp = rb_prev(&va->rb_node);
	if(tmp) {
		struct vmap_area *prev;
		
		prev = rb_entry(tmp,struct vmap_area,rb_node);
		list_add_rcu(&va->list,&prev->list);
	} else 
		list_add_rcu(&va->list,&vmap_area_list);
}

/*
 * Kick off a purge of the outstanding lazy areas.
 */
void purge_vmap_area_lazy(void)
{
	unsigned long start = ULONG_MAX,end = 0;

	__purge_vmap_area_lazy(&start,&end,1,0);
}

/*
 * Allocate a region of KVA of the specified size and alignment,within the
 * vstart and vend.
 */
static struct vmap_area *alloc_vmap_area(unsigned long size,
		unsigned long align,
		unsigned long vstart,unsigned long vend,
		int node,gfp_t gfp_mask)
{
	struct vmap_area *va;
	struct rb_node *n;
	unsigned long addr;
	int purged = 0;

	BUG_ON(!size);
	BUG_ON(size & ~PAGE_MASK);

	va = (struct vmap_area *)kmalloc_node(sizeof(struct vmap_area),
			gfp_mask & GFP_RECLAIM_MASK,node);
	if(unlikely(!va))
		return ERR_PTR(-ENOMEM);

retry:
	addr = ALIGN(vstart,align);

	spin_lock(&vmap_area_lock);
	if(addr + size - 1 < addr)
		goto overflow;

	/* XXX:could have a last_hole cache */
	n = vmap_area_root.rb_node;
	if(n) {
		struct vmap_area *first = NULL;

		do {
			struct vmap_area *tmp;
			tmp = rb_entry(n,struct vmap_area,rb_node);
			if(tmp->va_end >= addr) {
				if(!first && tmp->va_start < addr + size)
					first = tmp;
				n = n->rb_left;
			} else {
				first = tmp;
				n = n->rb_right;
			}
		} while(n);
		
		if(!first)
			goto found;

		if(first->va_end < addr) {
			n = rb_next(&first->rb_node);
			if(n)
				first = rb_entry(n,struct vmap_area,rb_node);
			else
				goto found;
		}
		
		while(addr + size > first->va_start && addr + size <= vend) {
			addr = ALIGN(first->va_end + PAGE_SIZE,align);
			if(addr + size - 1 < addr)
				goto overflow;

			n = rb_next(&first->rb_node);
			if(n)
				first = rb_entry(n,struct vmap_area,rb_node);
			else
				goto found;
		}
	}
found:
	if(addr + size > vend) {
overflow:
		spin_unlock(&vmap_area_lock);
		if(!purged) {
			purge_vmap_area_lazy();
			purged = 1;
			goto retry;
		}
		if(printk_ratelimit())
			mm_debug("Vmap allocation for size %p failed: "
					"use vmalloc=<size> to increase size.\n",
					(void *)size);
		kfree(va);
		return (struct vmap_area *)(unsigned long)ERR_PTR(-EBUSY);
	}

	BUG_ON(addr & (align - 1));

	va->va_start = addr;
	va->va_end   = addr + size;
	va->flags    = 0;
	__insert_vmap_area(va);
	spin_unlock(&vmap_area_lock);

	return va;
	
}
/*** Old vmalloc interfaces ***/

void insert_vmalloc_vm(struct vm_struct *vm,struct vmap_area *va,
		unsigned long flags,void *caller)
{
	struct vm_struct *tmp,**p;

	vm->flags = flags;
	vm->addr  = (void *)va->va_start;
	vm->size  = va->va_end - va->va_start;
	vm->caller = caller;
	va->private = vm;
	va->flags  |= VM_VM_AREA;

	write_lock(&vmlist_lock);
	for(p = &vmlist ; (tmp = *p) != NULL ; p = &tmp->next) {
		if(tmp->addr >= vm->addr)
			break;
	}
	vm->next = *p;
	*p = vm;
	write_unlock(&vmlist_lock);
}

static struct vm_struct *__get_vm_area_node(unsigned long size,
		unsigned long align,unsigned long flags,unsigned long start,
		unsigned long end,int node,gfp_t gfp_mask,
		void *caller)
{
	static struct vmap_area *va;
	struct vm_struct *area;

	BUG_ON(in_interrupt());
	if(flags & VM_IOREMAP) {
		int bit = fls(size);

		if(bit > IOREMAP_MAX_ORDER)
			bit = IOREMAP_MAX_ORDER;
		else if(bit < PAGE_SHIFT)
			bit = PAGE_SHIFT;

		align = 1UL << bit;
	}

	size = PAGE_ALIGN(size);
	if(unlikely(!size))
		return NULL;

	area = (struct vm_struct *)kzalloc_node(sizeof(*area),
				gfp_mask & GFP_RECLAIM_MASK,node);
	if(unlikely(!area))
		return NULL;

	/*
	 * We always allocate a guard page.
	 */
	size += PAGE_SIZE;

	va = alloc_vmap_area(size,align,start,end,node,gfp_mask);
	if(IS_ERR(va)) {
		kfree(area);
		return NULL;
	}

	insert_vmalloc_vm(area,va,flags,caller);
	return area;
}
void vfree(const void *addr);
static struct vmap_area *__find_vmap_area(unsigned long addr)
{
	struct rb_node *n = vmap_area_root.rb_node;

	while(n) {
		struct vmap_area *va;

		va = rb_entry(n,struct vmap_area,rb_node);
		if(addr < va->va_start)
			n = n->rb_left;
		else if(addr > va->va_start)
			n = n->rb_right;
		else
			return va;
	}
	
	return NULL;
}
static struct vmap_area *find_vmap_area(unsigned long addr)
{
	struct vmap_area *va;

	spin_lock(&vmap_area_lock);
	va = __find_vmap_area(addr);
	spin_unlock(&vmap_area_lock);

	return va;
}
static void vmap_debug_free_range(unsigned long start,unsigned long end)
{
}
/*
 * Free a vamp area,caller ensuring that the area has been unmapped
 * and flush_cache_vunmap had been called for the correct range
 * previoursly.
 */
static void free_vmap_area_noflush(struct vmap_area *va)
{
	va->flags |= VM_LAZY_FREE;
	atomic_add((va->va_end - va->va_start) >> PAGE_SHIFT, &vmap_lazy_nr);
	if(unlikely(atomic_read(&vmap_lazy_nr) > lazy_max_pages()))
		try_purge_vmap_area_lazy();
}
/*** Page table mainpulation functions ***/

static void vunmap_pte_range(pmd_t *pmd,unsigned long addr,unsigned long end)
{
	pte_t *pte;
#if 0
	pte = pte_offset_kernel(pmd,addr);
	do {
		pte_t ptent;
		ptent = ptep_get_and_clear(&init_mm,addr,pte);
		WARN_ON(!pte_none(ptent) && !pte_present(ptent));
	} while(pte++,addr += PAGE_SIZE , addr != end);
#endif
}
static void vunmap_pmd_range(pud_t *pud,unsigned long addr,unsigned long end)
{
	pmd_t *pmd;
	unsigned long next;

	pmd = pmd_offset(pud,addr);
	do {
		next = pmd_addr_end(addr,end);
		/* Need debug */
		//if(pmd_none_or_clear_bad(pmd))
		if(0)
			continue;
		vunmap_pte_range(pmd,addr,next);
	} while(pmd++,addr = next,addr != end);
}
static void vunmap_pud_range(pgd_t *pgd,unsigned long addr,unsigned long end)
{
	pud_t *pud;
	unsigned long next;

	pud = pud_offset(pgd,addr);
	do {
		next = pud_addr_end(addr,end);
		/* Need debug */
		//if(pud_none_or_clear_bad(pud))
		if(0)
			continue;
		vunmap_pmd_range(pud,addr,next);
	} while(pud++,addr = next,addr != end);
}
static void vunmap_page_range(unsigned long addr,unsigned long end)
{
	pgd_t *pgd;
	unsigned long next;

	BUG_ON(addr >= end);
	pgd = pgd_offset_k(addr);
	do {
		next = pgd_addr_end(addr,end);
		/* Need debug */
		//if(pgd_none_or_clear_bad(pgd))
		if(0)
			continue;
		vunmap_pud_range(pgd,addr,next);
	} while(pgd++,addr = next,addr != end);
}

static int vmap_pte_range(pmd_t *pmd,unsigned long addr,
		unsigned long end,pgprot_t prot,struct page **pages,
		int *nr)
{
	pte_t *pte;

	/*
	 * nr is running index into the array which helps higher level
	 * callers keep track of where we're up to.
	 */
   
	pte = pte_alloc_kernel(pmd,addr);
	if(!pte)
		return -ENOMEM;
	do {
		struct page *page = pages[*nr];
		
		if(WARN_ON(!pte_none(pte)))
			return -EBUSY;
		if(WARN_ON(!page))
			return -ENOMEM;
		mm_debug("MK_PTE %p\n",(pte_t *)(page_to_pfn(page) << PAGE_SHIFT));
		//set_pte_at(&init_mm,addr,pte,mk_pte(page,prot));
		(*nr)++;
		
	} while(pte++,addr += PAGE_SIZE,addr != end);
}
static int vmap_pmd_range(pud_t *pud,unsigned long addr,
		unsigned long end,pgprot_t prot,struct page **pages,int *nr)
{
	pmd_t *pmd;
	unsigned long next;

	pmd = pmd_alloc(&init_mm,pud,addr);
	if(!pmd)
		return -ENOMEM;
	do {
		next = pmd_addr_end(addr,end);
		if(vmap_pte_range(pmd,addr,next,prot,pages,nr))
			return -ENOMEM;
	} while(pmd++,addr = next,addr != end);
	return 0;
}

static int vmap_pud_range(pgd_t *pgd,unsigned long addr,
		unsigned long end,pgprot_t prot,struct page **pages,int *nr)
{
	pud_t *pud;
	unsigned long next;

	pud = pud_alloc(&init_mm,pgd,addr);
	if(!pud)
		return -ENOMEM;
	do {
		next = pud_addr_end(addr,end);
		if(vmap_pmd_range(pud,addr,next,prot,pages,nr))
			return -ENOMEM;
	} while(pud++,addr = next,addr != end);

	return 0;
}

/*
 * Set up page tables in kva(addr,end).The ptes shall have prot "prot",and
 * will have pfns corresponding to the "pages" array.
 *
 * le.pte at addr+N*PAGE_SIZE shall point to pfn corresponding to pages[N].
 */
static int vmap_page_range_noflush(unsigned long start,unsigned long end,
		pgprot_t prot,struct page **pages)
{
	pgd_t *pgd;
	unsigned long next;
	unsigned long addr = start;
	int err = 0;
	int nr = 0;

	BUG_ON(addr >= end);
	pgd = pgd_offset_k(addr);
	do {
		next = pgd_addr_end(addr,end);
		err  = vmap_pud_range(pgd,addr,next,prot,pages,&nr);
		if(err)
			return err;
	} while(pgd++,addr = next,addr != end);

	return nr;
}
/*
 * Clear the pagetable entries of a given vmap_area.
 */
static void unmap_vmap_area(struct vmap_area *va)
{
	vunmap_page_range(va->va_start,va->va_end);
}
/*
 * Free and unmap a vmap area,caller ensuring flush_cache_vunmap had been
 * called for the correct range previously.
 */
static void free_unmap_vmap_area_noflush(struct vmap_area *va)
{
	unmap_vmap_area(va);
	free_vmap_area_noflush(va);
}
/*
 * Free and unmap a vmap area.
 */
static void free_unmap_vmap_area(struct vmap_area *va)
{
	flush_cache_vunmap(va->va_start,va->va_end);
	free_unmap_vmap_area_noflush(va);
}
/*
 * Find and remove a continuous kernel virtual area.
 */
struct vm_struct *remove_vm_area(const void *addr)
{
	struct vmap_area *va;

	va = find_vmap_area((unsigned long)addr);
	if(va && va->flags & VM_VM_AREA)
	{
		struct vm_struct *vm = va->private;
		struct vm_struct *tmp,**p;

		/*
		 * Remove from list and disallow access to this vm_struct
		 * before unmap.(address range confiliction is maintained by
		 * vmap.)
		 */
		write_lock(&vmlist_lock);
		for(p = &vmlist ; (tmp = *p) != vm ; p = &tmp->next)
			;
		*p = tmp->next;
		write_unlock(&vmlist_lock);

		vmap_debug_free_range(va->va_start,va->va_end);
		free_unmap_vmap_area(va);
		vm->size -= PAGE_SIZE;

		return vm;
	}
	return NULL;
}

static int vmap_page_range(unsigned long start,unsigned long end,
		pgprot_t prot,struct page **pages)
{
	int ret;

	ret = vmap_page_range_noflush(start,end,prot,pages);
	flush_cache_vmap(start,end);
	
	return ret;
}

int map_vm_area(struct vm_struct *area,pgprot_t prot,struct page ***pages)
{
	unsigned long addr = (unsigned long)area->addr;
	unsigned long end  = addr + area->size - PAGE_SIZE;
	int err;

	err = vmap_page_range(addr,end,prot,*pages);
	if(err > 0) {
		*pages += err;
		err = 0;
	}

	return err;
}

static void *__vmalloc_node(unsigned long size,unsigned long align,
				gfp_t gfp_mask,pgprot_t prot,
						int node,void *caller);
static void *__vmalloc_area_node(struct vm_struct *area,gfp_t gfp_mask,
		pgprot_t prot,int node,void *caller)
{
	struct page **pages;
	unsigned int nr_pages,array_size,i;
	gfp_t nested_gfp = (gfp_mask & GFP_RECLAIM_MASK) | __GFP_ZERO;

	nr_pages = (area->size - PAGE_SIZE) >> PAGE_SHIFT;
	array_size = (nr_pages * sizeof(struct page *));

	area->nr_pages = nr_pages;
	/* Pls not that the recursion is strictly bounded. */
	if(array_size > PAGE_SIZE) {
		pages = __vmalloc_node(array_size,1,nested_gfp | __GFP_HIGHMEM,
				PAGE_KERNEL,node,caller);
	
		area->flags |= VM_VPAGES;
	} else {
		pages = (struct page **)(unsigned long)kmalloc_node(array_size,
				nested_gfp,node);
	}
	area->pages = pages;
	area->caller = caller;
	if(!area->pages) {
		remove_vm_area(area->addr);
		kfree(area);
		return NULL;
	}

	for(i = 0 ; i < area->nr_pages ; i++) {
		struct page *page;

		if(node < 0)
			page = alloc_page(gfp_mask);
		else
			page = alloc_pages_node(node,gfp_mask,0);

		if(unlikely(!page)) {
			/* Successfuly allocated i pages,free them in __vunmap() */
			area->nr_pages = i;
			goto fail;
		}
		area->pages[i] = page;
	}
	
	if(map_vm_area(area,prot,&pages))
		goto fail;
	return area->addr;

fail:
	vfree(area->addr);
	return NULL;
}

/**
 * __vmalloc_node_range - allocate virtually contiguous memory
 * @size:              allocation size
 * @align:             desired alignment
 * @start:             vm area range start
 * @end:               vm area range end
 * @gfp_mask:          flags for the page level allocator
 * @prot:              protection mask for the allocated pages.
 * @node:              node to use for allocation or -1
 * @caller:            caller's return address.
 *
 * Allocate enough pages to cover @size from the page level
 * allocator with @gfp_mask flags.Map them into contiguous
 * kernel virtual space,using a pagetable protect of @prot.
 */
void *__vmalloc_node_range(unsigned long size,unsigned long align,
		unsigned long start,unsigned long end,gfp_t gfp_mask,
		pgprot_t prot,int node,void *caller)
{
	struct vm_struct *area;
	void *addr;
	unsigned long real_size = size;

	size = PAGE_ALIGN(size);
	if(!size || (size >> PAGE_SHIFT) > totalram_pages)
		return NULL;

	area = __get_vm_area_node(size,align,VM_ALLOC,start,end,node,
			gfp_mask,caller);

	if(!area)
		return NULL;

	addr = __vmalloc_area_node(area,gfp_mask,prot,node,caller);

	/*
	 * A ref_count = 3 is needed because the vm_struct and vmap_area
	 * structures allocated in the __get_vm_area_node() funciton contain
	 * references to the virtual address of the vmalloc'ed block.
	 */
	kmemleak_alloc(addr,real_size,3,gfp_mask);

	return addr;
}

/**
 * __vmalloc_node - allocate virtually contiguous memory
 * @size:            allocation size
 * @align:           desired alignment
 * @gfp_mask:        flags for the page level allocator
 * @prot:            protection mask for the allocated pages
 * @node:            node to use for allocation or -1
 * @caller:          caller's return address.
 *
 * Allocate enough pages to cover @size from the page level
 * allocator with @gfp_mask flags.Map them into contigous
 * kernel virtual space,using a pagetable protection of @prot.
 */
static void *__vmalloc_node(unsigned long size,unsigned long align,
		gfp_t gfp_mask,pgprot_t prot,
		int node,void *caller)
{
	return __vmalloc_node_range(size,align,VMALLOC_START,VMALLOC_END,
			gfp_mask,prot,node,caller);
}

static inline void *__vmalloc_node_flags(unsigned long size,
		int node,gfp_t flags)
{
	return __vmalloc_node(size,1,flags,PAGE_KERNEL,
			node,__builtin_return_address(0));
}

/*
 * vmalloc - allocate virtually contiguous memory
 * @size: allocation size
 * Allocate enough pages to cover @size from the page level
 * allocator and map them into contiguous kernel virtual space.
 *
 * For tight control over page level allocator and protection flags
 * use __vmalloc() instead.
 */
void *vmalloc(unsigned long size)
{
	return __vmalloc_node_flags(size,-1,GFP_KERNEL | __GFP_HIGHMEM);
}

/*
 * Allocate virtually contiguous memory with zero fill
 */
void *vzalloc(unsigned long size)
{
	return __vmalloc_node_flags(size,-1,
			GFP_KERNEL | __GFP_HIGHMEM | __GFP_ZERO);
}

static void rcu_free_vb(struct rcu_head *head)
{
	struct vmap_block *vb = container_of(head,struct vmap_block,rcu_head);

	kfree(vb);
}
static void __vunmap(const void *addr,int deallocate_pages)
{
	struct vm_struct *area;

	if(!addr)
		return;

	if((PAGE_SIZE - 1) & (unsigned long)addr)
	{
		mm_warn("Trying to vfree() bad address\n");
		return;
	}

	area = remove_vm_area(addr);
	if(unlikely(!area))
	{
		mm_warn("Trying to vfree() nonexistent vm area\n");
		return;
	}
	/* Need debug */	
//	debug_check_no_locks_freed(addr,area->size);
//	debug_check_no_obj_freed(addr,area->size);

	if(deallocate_pages) {
		int i;

		for(i = 0 ; i < area->nr_pages ; i++) {
			struct page *page = area->pages[i];
			
			BUG_ON(!page);
			__free_page(page);
		}

		if(area->flags & VM_VPAGES)
			vfree(area->pages);
		else
			kfree(area->pages);
	}
	kfree(area);
	return;
}
/*
 * vfree - release memory allocator by vmalloc()
 */
void vfree(const void *addr)
{
	BUG_ON(in_interrupt());

	kmemleak_free(addr);

	__vunmap(addr,1);
}
struct vmap_area *node_to_va(struct rb_node *n)
{
	return n ? rb_entry(n,struct vmap_area,rb_node) : NULL;
}
/*
 * Find the highest aligned address between two vmap_areas
 */
unsigned long pvm_determine_end(struct vmap_area **pnext,
		struct vmap_area **pprev,
		unsigned long align)
{
	const unsigned long vmalloc_end = VMALLOC_END & ~(align - 1);
	unsigned long addr;

	if(*pnext)
		addr = min((*pnext)->va_start & ~(align - 1),vmalloc_end);
	else
		addr = vmalloc_end;

	while(*pprev && (*pprev)->va_end > addr)
	{
		*pnext = *pprev;
		*pprev = node_to_va(rb_prev(&(*pnext)->rb_node));
	}

	return addr;
}
/*
 * Find the next and prev vmap_area surrounding @end
 */
bool pvm_find_next_prev(unsigned long end,
		struct vmap_area **pnext,
		struct vmap_area **pprev)
{
	struct rb_node *n = vmap_area_root.rb_node;
	struct vmap_area *va = NULL;

	while(n)
	{
		va = rb_entry(n,struct vmap_area,rb_node);
		if(end < va->va_end)
			n = n->rb_left;
		else if(end > va->va_end)
			n = n->rb_right;
		else
			break;
	}

	if(!va)
		return false;

	if(va->va_end > end)
	{
		*pnext = va;
		*pprev = node_to_va(rb_prev(&(*pnext)->rb_node));
	} else
	{
		*pprev = va;
		*pnext = node_to_va(rb_next(&(*pprev)->rb_node));
	}
	return true;
}
/*
 * Map kernel VM area with the specified pages.
 */
int map_kernel_range_noflush(unsigned long addr,unsigned long size,
		pgprot_t prot,struct page **pages)
{
	return vmap_page_range_noflush(addr,addr + size,prot,pages);
}
/*
 * Unmap kernel VM area
 */
void unmap_kernel_range_noflush(unsigned long addr,unsigned long size)
{
	vunmap_page_range(addr,addr + size);
}
/*
 * Walk a vmap address to the struct page it maps.
 */
struct page *vmalloc_to_page(const void *vmalloc_addr)
{
	unsigned long addr = (unsigned long)vmalloc_addr;
	struct page *page = NULL;
	pgd_t *pgd = pgd_offset_k(addr);

	/*
	 * XXX we might need to change this if we add VIRTUAL_BUG_ON for 
	 * architechure that do not vmalloc module space
	 */
	VIRTUAL_BUG_ON(!is_vmalloc_or_module_addr(vmalloc_addr));

	if(!pgd_none(pgd)) {
		pud_t *pud = pud_offset(pgd,addr);
		if(!pud_none(pud)) {
			pmd_t *pmd = pmd_offset(pud,addr);
			if(!pmd_none(pmd)) {
				pte_t *ptep,pte;
				/* Need rebuilt */
				//ptep = pte_offset_map(pmd,addr);
				pte = *ptep;
				if(pte_present(pte))
					page = (struct page *)(unsigned long)pte_page(pte);
				pte_unmap(ptep);
			}
		}
	}
	return page;
}

void __init vmalloc_init(void)
{
	struct vmap_area *va;
	struct vm_struct *tmp;
	int i;

	for_each_possible_cpu(i) {
		struct vmap_block_queue *vbq;
		
		vbq = &per_cpu(vmap_block_queue,i);
		spin_lock_init(&vbq->lock);
		INIT_LIST_HEAD(&vbq->free);
	}

	/* Import exiting vmlist entries */
	for(tmp = vmlist ; tmp ; tmp = tmp->next) {
		va = kzalloc(sizeof(struct vmap_area),GFP_NOWAIT);
		va->flags = tmp->flags | VM_VM_AREA;
		va->va_start = (unsigned long)tmp->addr;
		va->va_end = va->va_start + tmp->size;
		__insert_vmap_area(va);
	}

	vmap_area_pcpu_hole = VMALLOC_END;

	vmap_initialized = true;
}



