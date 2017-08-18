// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if((err & FEC_WR) == 0 || (((pte_t*)uvpt)[PGNUM(addr)] & PTE_COW) == 0)
		panic("pgfault : the faulting access is not a write op.");
	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	
	// LAB 4: Your code here.
	//panic("pgfault not implemented");
	//cprintf("thisenv->env_id = %8x\n",thisenv->env_id);
	//cprintf("curenv->env_id = %8x\n",sys_getenvid());
	//不要用thisenv->env_id,还未更新
	if((r = sys_page_alloc(0, (void*)PFTEMP, PTE_P | PTE_U |PTE_W)) < 0)
		panic("sys_page_alloc: %e.",r);
	addr = ROUNDDOWN(addr, PGSIZE);
	memmove((void*)PFTEMP, addr, PGSIZE);
	if((r = sys_page_map(0,  (void*)PFTEMP, 0, ROUNDDOWN(addr, PGSIZE), PTE_P | PTE_U |PTE_W)) < 0)
		panic("sys_page_map: %e.",r);
	if((r = sys_page_unmap(0, (void*)PFTEMP)) < 0)
		panic("sys_page_unmap: %e.", r);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	//panic("duppage not implemented");
	//uint32_t perm;
	//perm = PGOFF(((pte_t*)uvpt)[pn]);
	pte_t pte = ((pte_t*)uvpt)[pn];
	if(pte & PTE_SHARE){
		if((r = sys_page_map(0, (void*)(pn * PGSIZE), envid, (void*)(pn * PGSIZE), pte & PTE_SYSCALL)) < 0)
			return r;
	}
	else if(pte & (PTE_W | PTE_COW)){
		//perm = perm &(~PTE_W);
		if((r = sys_page_map(0, (void*)(pn * PGSIZE), envid, (void*)(pn * PGSIZE), PTE_P |PTE_U| PTE_COW)) < 0)
			return r;
		if((r = sys_page_map(0, (void*)(pn * PGSIZE), 0, (void*)(pn * PGSIZE), PTE_P |PTE_U | PTE_COW)) < 0)
			return r;
	}
	else{
		if((r = sys_page_map(0, (void*)(pn * PGSIZE), envid, (void*)(pn * PGSIZE), PTE_P |PTE_U )) < 0)
			return r;
	}
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	//panic("fork not implemented");
	set_pgfault_handler(pgfault);
	envid_t envid;
	envid = sys_exofork();
	if(envid < 0)
		panic("sys_exofork: %e.",envid);
	if(envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
		//cprintf("thisenv->env_id = %8x\n",thisenv->env_id);
		return 0;
	}
	int i, j;
	int r;
	unsigned pn;
	for(i = PDX(UTEXT); i < PDX(UTOP); i++){
		if(((pde_t*)uvpd)[i] & PTE_P){
			for(j = 0; j < NPTENTRIES; j++){
				pn = PGNUM(PGADDR(i, j, 0));
				if(pn == PGNUM(UXSTACKTOP - PGSIZE)) //注意此步为跳过异常堆栈页
					break;
				if(((pte_t*)uvpt)[pn] & PTE_P)
					if((r = duppage(envid, pn)) < 0)
						panic("duppage: %e",r);
			}
		}
	}
	if ((r = sys_page_alloc(envid, (void*)(UXSTACKTOP - PGSIZE), PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_alloc: %e", r);
	if ((r = sys_page_map(envid, (void*)(UXSTACKTOP - PGSIZE), 0, UTEMP, PTE_P|PTE_U|PTE_W)) < 0)
		panic("sys_page_map: %e", r);
	memmove(UTEMP, (void*)(UXSTACKTOP - PGSIZE), PGSIZE);
	if ((r = sys_page_unmap(0, UTEMP)) < 0)
		panic("sys_page_unmap: %e", r);
	extern void _pgfault_upcall(void);
	if((r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall)) < 0)
		panic("sys_env_set_pgfault_upcall: %e", r);
	if((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);
	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
