#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/string.h>
// LAB 6: Your driver code here

volatile uint32_t *e1000; //pointer to the device register mapping
volatile uint32_t *e1000_tdt;
volatile uint32_t *e1000_rdt;

struct tx_desc tx_desc_list[MAXTDLLEN];
char t_buf[MAXTDLLEN][MAXBUFSIZE];

struct rx_desc rx_desc_list[MAXRDLLEN];
char r_buf[MAXRDLLEN][MAXBUFSIZE];

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
	//cprintf("E1000_TDBAL = 0x%x\n",PADDR(tx_desc_list));
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
	/*struct tx_desc mytd = {0xf0271f00, 0x2a, 0, 0x9, 0,0, 0};
	while(1){
		if(e1000_transmit(&mytd) < 0)
			continue;
		else
			return 0;
	}*/
	
	//receive initialization
	e1000[E1000_RA/4] = RAL;
	e1000[E1000_RA/4 + 1] = RAH | E1000_RAH_AV;
	e1000[E1000_MTA/4] = 0;
	//e1000[E1000_IMS/4] = 0;
	struct rx_desc rd_init = {
    .addr = 0,
    .length = 0,
    .csum = 0,
    .status = 0,
    .errors = 0,
    .special = 0,
	};
	for(i = 0; i < MAXRDLLEN; i++){
		rx_desc_list[i] = rd_init;
	}
	e1000[E1000_RDBAL/4] = PADDR(rx_desc_list);
	e1000[E1000_RDBAH/4] = 0;
	e1000[E1000_RDLEN/4] = sizeof(rx_desc_list);
	memset(r_buf, 0, sizeof(r_buf));
	for(i = 0; i < MAXRDLLEN; i++){
		rx_desc_list[i].addr = PADDR(r_buf[i]);
	}
	e1000[E1000_RDH/4] = 0;
	e1000[E1000_RDT/4] = MAXRDLLEN - 1;
	e1000_rdt = e1000 + E1000_RDT/4;
	uint32_t rctl_flag = 0;
	rctl_flag |= E1000_RCTL_EN;
	//rctl_flag |= E1000_RCTL_LPE;  //接收巨型帧
	rctl_flag |= E1000_RCTL_LBM_NO;
	//RCTL.RDMTS:最小阈值
	//组播偏移（RCTL.MO）位配置为所需值
	rctl_flag |= E1000_RCTL_BAM;
	rctl_flag |= E1000_RCTL_SZ_2048 ;
	//rctl_flag |= E1000_RCTL_BSEX;
	//rctl_flag |= E1000_RCTL_SZ_4096;
	
	e1000[E1000_RCTL/4] = rctl_flag;
	
	return 0;
}

int
e1000_transmit(struct tx_desc *td){
	struct tx_desc *ntd = tx_desc_list + *e1000_tdt;
	if((ntd->status & (E1000_TXD_STAT_DD >> E1000_TXD_STAT_SHIFT)) == 0){
		cprintf("transmit descriptor list is full\n");
		return -1;
	}
	*ntd = *td;
	memmove((void*)t_buf[*e1000_tdt], (void*)(uint32_t)ntd->addr, ntd->length);
	//cprintf("e1000_transmit: ntd->length = %d\n",ntd->length);
	ntd->addr = PADDR(t_buf[*e1000_tdt]);
	ntd->cmd |= (E1000_TXD_CMD_RS >> E1000_TXD_CMD_SHIFT);
	*e1000_tdt = (*e1000_tdt + 1) % MAXTDLLEN; 
	return 0;
}

int
e1000_receive(struct rx_desc *rd){
	int i = (*e1000_rdt +1) % MAXRDLLEN;
	struct rx_desc *nrd = rx_desc_list + i;
	if((nrd->status & (E1000_RXD_STAT_DD >> E1000_RXD_STAT_SHIFT)) == 0){
		//cprintf("receive free\n");
		return -1;
	}
	uint64_t bufaddr = rd->addr;
	*rd = *nrd;
	rd->addr = bufaddr;
	//问题：为何length需要-4（sizeof(int)）才能通过test？？？
	rd->length -= 4;
	//注意！！memmove与中用的是虚拟地址。rd->addr是虚地址，而nrd->addr是物理地址
	memmove((void*)(uint32_t)rd->addr, (void*)r_buf[i], rd->length);
	nrd->status = 0;
	*e1000_rdt = i;
	return 0;
}
