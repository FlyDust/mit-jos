// Fork a binary tree of processes and display their structure.

/*#include <inc/lib.h>

#define DEPTH 3

void forktree(const char *cur);

void
forkchild(const char *cur, char branch)
{
	char nxt[DEPTH+1];

	if (strlen(cur) >= DEPTH)
		return;

	snprintf(nxt, DEPTH+1, "%s%c", cur, branch);
	if (fork() == 0) {
		forktree(nxt);
		exit();
	}
}

void
forktree(const char *cur)
{
	cprintf("%04x: I am '%s'\n", sys_getenvid(), cur);

	forkchild(cur, '0');
	forkchild(cur, '1');
}

void
umain(int argc, char **argv)
{
	forktree("");
}*/
#include <inc/lib.h>
void umain(int argc,char **argv)
{
	cprintf("hi,father\n");
	cprintf("%04x\n", sys_getenvid());
	int id1,id2;
	id1 = fork();
	id2 = fork();
	if(id1==0){
		cprintf("hi,son1\n");
		cprintf("%04x\n", sys_getenvid());
	}
	if(id2==0){
		cprintf("hi,son2\n");
		cprintf("%04x\n", sys_getenvid());
		exit();
	}
}

