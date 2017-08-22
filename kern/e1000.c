#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/string.h>
// LAB 6: Your driver code here

volatile uint32_t *e1000; //pointer to the device register mapping
volatile uint32_t *e1000_tdt;

struct tx_desc tx_desc_list[MAXTDLLEN];
char pack_buf[MAXTDLLEN][MAXPACKETSIZE];

int
e1000_attach(struct pci_func *pcif){
	//enable the device
	pci_func_enable(pcif);
	//create a virtual memory mapping for the E1000's BAR 0
	e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	//打印状态寄存器的值
	cprintf("e1000_status = 0x%x\n", e1000[E1000_STATUS/4]);
	//Transmit Initialization
	struct tx_desc td_init = {
		.addr = 0,
		.length = 0,
		.cso = 0,
		//.cmd = E1000_TXD_CMD_RS >> E1000_TXD_CMD_SHIFT,
		.cmd = 0,
		.status = E1000_TXD_STAT_DD >> E1000_TXD_STAT_SHIFT,
		.css = 0,
		.special = 0,
	};
	int i;
	for(i = 0;i < MAXTDLLEN; i++){
		tx_desc_list[i] = td_init;
	}
	e1000[E1000_TDBAL/4] = PADDR(tx_desc_list);
	cprintf("E1000_TDBAL = 0x%x\n",PADDR(tx_desc_list));
	e1000[E1000_TDBAH/4] = 0;
	e1000[E1000_TDLEN/4] = sizeof(tx_desc_list);
	e1000[E1000_TDH/4] = 0;
	e1000[E1000_TDT/4] = 0;
	e1000_tdt = e1000 + E1000_TDT/4;
	uint32_t tctl_flag = 0;
	tctl_flag |= E1000_TCTL_EN;
	tctl_flag |= E1000_TCTL_PSP;
	tctl_flag |= (0x40 << 12);
	e1000[E1000_TCTL/4] = tctl_flag;
	uint32_t tipg_flag = 0;
	tipg_flag |= 10;
	tipg_flag |= (4 << 10);
	tipg_flag |= (6 << 20);
	e1000[E1000_TIPG/4] = tipg_flag;
	
	//test transmit
	struct tx_desc mytd = {0x271f00, 0x2a, 0, 0x90, 0,0, 0};
	memset((void*)0x271f00, 1, 100);
	while(1){
		if(e1000_transmit(&mytd) < 0)
			continue;
		else
			return 0;
	}
	return 0;
}

int
e1000_transmit(struct tx_desc *td){
	struct tx_desc *ntd = tx_desc_list + *e1000_tdt;
	if((ntd->status & (E1000_TXD_STAT_DD >> E1000_TXD_STAT_SHIFT)) == 0){
		return -1;
	}
	*ntd = *td;
	//ntd->cmd |= (E1000_TXD_CMD_RS >> E1000_TXD_CMD_SHIFT);
	*e1000_tdt = (*e1000_tdt + 1) % MAXTDLLEN; 
	return 0;
}
