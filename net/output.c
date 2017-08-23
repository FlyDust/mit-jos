#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	uint32_t req, whom;
	int r;
	struct tx_desc td = {0};
	while(1){
		req = ipc_recv((int32_t*)&whom, &nsipcbuf, 0);
		if(req < 0)
			panic("in output, ipc_recv: %e",r);
		if (whom != ns_envid) {
			cprintf("NS OUTPUT: output environment got IPC message from env %x not NS\n", whom);
			continue;
		}
		if(req != NSREQ_OUTPUT){
			cprintf("NS OUTPUT:output environment got IPC message %d not NSREQ_OUTPUT\n",req);
			continue; 
		}
		if(req == NSREQ_OUTPUT){
			td.addr = (uint32_t)nsipcbuf.pkt.jp_data;
			td.length = MIN(nsipcbuf.pkt.jp_len, sizeof(union Nsipc) - sizeof(int));
			td.cmd = 9;
			while(1){
				r = sys_e1000_trans(&td);
				if(r < 0)
					continue;
				break;
			}
		}
	}
}
