#include "ns.h"

extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	int r,i=0;
	while(1) {
		struct rx_desc rd = {0,0,0,0,0,0};
		//don't immediately receive another packet in to the same physical page.重新分配映射
		if((r = sys_page_alloc(0, &nsipcbuf, PTE_P | PTE_U | PTE_W)) < 0)
			panic("in net/input.c, sys_page_alloc: %e", r);
		rd.addr = (uint32_t)nsipcbuf.pkt.jp_data;
		while(1){
			r = sys_e1000_recv(&rd);
			if(r < 0)
				sys_yield();
				//continue;
			else
				break;
		}
		nsipcbuf.pkt.jp_len = rd.length;
		//cprintf("nsipcbuf.pkt.jp_len = %d\n",nsipcbuf.pkt.jp_len);
		ipc_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_P | PTE_U | PTE_W);
	}
}
