/*
 *  linux/arch/arm/oprofile/aop_elf_parser.c
 *
 *  ELF Symbol Table Parsing Module
 *
 *  Copyright (C) 2009  Samsung	
 *
 *  2009-07-21  Created by gaurav.j@samsung.com
 *
 */

/********************************************************************
 INCLUDE FILES
********************************************************************/
#include <linux/types.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/unistd.h>
#include <linux/proc_fs.h>
#include <linux/kdebugd.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/file.h>
#include <linux/vmalloc.h>
#include <linux/bootmem.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/nmi.h>
#include <linux/elf.h>

#include "aop_elf_def.h"
#include "aop_sym_db.h"

/*  Define ARRAY, if you want to pass elf file as an array 
*    else dont define
*/
#ifdef ARRAY
#include "Exe_Temp.h"
#endif

/********************************************************************
 MACRO CONSTANT DEFINITIONS
********************************************************************/

/*debug logs will be on if DEBUG_LOGS is 1 otherwise OFF*/
#define IS_DEBUG_ON 0
#if IS_DEBUG_ON
#define aop_printk(fmt,args...) printk(fmt,##args)
#else
#define aop_printk(fmt,...)
#endif /* IS_DEBUG_ON */

#define aop_errk(fmt,args...) printk("ERR: %s:%d:%s() " fmt, \
				__FILE__,__LINE__, __FUNCTION__ ,##args)

#define AOP_SYM_MAX_SO_LIBS  64

#define AOP_SYM_MAX_SO_LIB_PATH_LEN 128


/* 
*	Pre defined directory path in USB where ELF should be placed
*/
#define AOP_ELF_PATH "aop_bin" 

/*
 *	This member gives the section’s size in bytes of Section Table
*/
extern int  aop_config_additive;

 /*  
 *Symbol name parsing  
 */
#define AOP_SYM_NAME(X,Y,Z)	\
  ((X) == NULL ? "<none>" \
  : (Y) == NULL ? "<no-name>" \
  : ((X)->st_name >= (Z) ? "<corrupt>" \
  : (Y) + (X)->st_name))

/* 
*Dynamic Symbol name parsing 
*/
#define AOP_DYN_SYM_NAME(X,Y,Z)	\
  ((X) == NULL ? "<none>" \
  : (Y) == NULL ? "<no-name>" \
  : ((X)->st_name >= (Z) ? "<corrupt>" \
  : (Y) + (X)->st_name))

/* Print a VMA value.  */
/* How to print a vma value.  */
/* Represent a target address.  Also used as a generic unsigned type
   which is guaranteed to be big enough to hold any arithmetic types
   we need to deal with.  */
typedef unsigned long aop_pvma;

typedef enum aop_print_mode
{
  HEX,
  DEC,
  DEC_5,
  UNSIGNED,
  PREFIX_HEX,
  FULL_HEX,
  LONG_HEX
}aop_print_mode;

/*
* This Structure Holds the Symbol related information
*/
typedef struct 
{
	struct list_head	sym_list;
/* Symbol name, index in string tbl */	
  	unsigned int st_name;		
/* Type and binding attributes */
  	unsigned char	st_info;
/* Value of the symbol */
	unsigned int  st_value;		
}aop_kernel_symbol_item ;

/*
* This Structure Holds the total no ELF files in USB
*/
typedef struct 
{
	struct list_head	usb_elf_list;
/* ELF names detect from USB */	
  	char elf_name[AOP_MAX_ELF_FILE_NAME_LEN];	
/* Total no of ELF found in USB */
	int elf_entry_no;	
/* Total no of ELF Symbol Found in the directory */
	int elf_symbol_count;
/* Holds Symbol name */
	char* sym_buff;
/* Holds Symbol name */
	char* dyn_sym_buff;
/* Hold the total sym buff size*/
	uint32_t sym_str_size;
/* Hold the total dyn buff size*/
	uint32_t dyn_str_size;
/* USB Path */
       char path_name[AOP_MAX_PATH_LEN];
/* Virtual Addr */
	uint32_t virtual_addr;
/* file Type */
	uint16_t file_type;
}aop_usb_elf_list_item;

struct aop_fs_lib_paths
{
    char libPath[AOP_SYM_MAX_SO_LIBS][AOP_SYM_MAX_SO_LIB_PATH_LEN];
    int num_paths;
};

/*
* This member holds the head point of ELF name stored in database of ELF files
*/
LIST_HEAD(aop_usb_aop_head_item);

/*
* This member holds the head point of Symbol related info
*/
static struct list_head *aop_head_item[AOP_MAX_FILES];

aop_symbol_load_notification aop_sym_load_notification_func = NULL;

#if IS_DEBUG_ON
static char *aop_get_file_type (unsigned e_type)
{
  static char buff[32];

  switch (e_type)
    {
    case ET_NONE:	return ("NONE (None)");
    case ET_REL:	return ("REL (Relocatable file)");
    case ET_EXEC:	return ("EXEC (Executable file)");
    case ET_DYN:	return ("DYN (Shared object file)");
    case ET_CORE:	return ("CORE (Core file)");

    default:
      if ((e_type >= ET_LOPROC) && (e_type <= ET_HIPROC))
	snprintf (buff, sizeof (buff), ("Processor Specific: (%x)"), e_type);
      else if ((e_type >= ET_LOOS) && (e_type <= ET_HIOS))
	snprintf (buff, sizeof (buff), ("OS Specific: (%x)"), e_type);
      else
	snprintf (buff, sizeof (buff), ("<unknown>: %x"), e_type);
      return buff;
    }
}
#endif

static const char *aop_get_symbol_binding (unsigned int binding)
{
  static char buff[32];

  switch (binding)
    {
    case AOP_STB_LOCAL:	return "LOCAL";
    case AOP_STB_GLOBAL:	return "GLOBAL";
    case AOP_STB_WEAK:	return "WEAK";
    default:
      if (binding >= AOP_STB_LOPROC && binding <= AOP_STB_HIPROC)
	snprintf (buff, sizeof (buff), ("<processor specific>: %d"),
		  binding);
      else if (binding >= AOP_STB_LOOS && binding <= AOP_STB_HIOS)
	snprintf (buff, sizeof (buff), ("<OS specific>: %d"), binding);
      else
	snprintf (buff, sizeof (buff), ("<unknown>: %d"), binding);
      return buff;
    }
}

static const char *aop_get_symbol_type (unsigned int type)
{
	static char buff[32];

	switch (type)
	{
		case AOP_STT_NOTYPE:	return "NOTYPE";
		case AOP_STT_OBJECT:	return "OBJECT";
		case AOP_STT_FUNC:	return "FUNC";
		case AOP_STT_SECTION:	return "SECTION";
		case AOP_STT_FILE:	return "FILE";
		case AOP_STT_COMMON:	return "COMMON";
		case AOP_STT_TLS:	return "TLS";
		//    case AOP_STT_RELC:      return "RELC";
		//    case AOP_STT_SRELC:     return "SRELC";
		default:
		snprintf (buff, sizeof (buff), ("<unknown>: %d"), type);
		return buff;
	}
}

static int aop_print_vma (aop_pvma vma, aop_print_mode mode)
{
	switch (mode)
	{
		case FULL_HEX:
	  	return printk ("0x%8.8lx", (unsigned long) vma);

		case LONG_HEX:
	  	return printk ("%8.8lx", (unsigned long) vma);

		case DEC_5:
	  		if (vma <= 99999)
	    		return printk ("%5ld", (long) vma);
	  	/* Drop through.  */

		case PREFIX_HEX:
	  	return printk ("0x%lx", (unsigned long) vma);

		case HEX:
	  	return printk ("%lx", (unsigned long) vma);

		case DEC:
	  	return printk ("%ld", (unsigned long) vma);

		case UNSIGNED:
	  	return printk ("%lu", (unsigned long) vma);
	}

	return 0;
}

/*
 *   Add the Symbol info in Symbol database
 */
static int aop_sym_add(unsigned int* value, 
		unsigned int* name, unsigned char* info, int* list_num)
{
	aop_kernel_symbol_item *item;

	item= kmalloc(sizeof(aop_kernel_symbol_item), GFP_KERNEL);
	if(!item){
		aop_errk("item --isyms--out of memory\n");
		return -ENOMEM;
	}
	item->st_info = *info;
	item->st_name = *name;
	item->st_value =*value;

	list_add_tail(&item->sym_list, aop_head_item[*list_num]);
	return 0;
}

/*
 *   Symbol info related datbase deletion  from the link list 
 */
static void aop_sym_delete_single_elf_db(int* list_num)
{
	aop_kernel_symbol_item *t_item;
	struct list_head *elf_pos, *q;

	aop_printk("[%s] : [%d]\n", __FUNCTION__, __LINE__);

	aop_printk("List No : %d\n", *list_num);
	aop_printk("[%s] : aop_head_item[%d] :: %p\n", __FUNCTION__,
				*list_num, aop_head_item[*list_num]);

	list_for_each_safe(elf_pos, q, aop_head_item[*list_num]) {
		t_item = list_entry(elf_pos, aop_kernel_symbol_item, sym_list);
		if(!t_item){
			printk("[%s] : t_item is NULL\n", __FUNCTION__);	
			return ;			
		}			 
		list_del(elf_pos);
		kfree(t_item);
	}
}

/*
 *   Parse the ELF file
 */
static int aop_sym_read_elf(
	char *filename, aop_usb_elf_list_item *plist, int* list_num)
{
	int idx;
	Elf32_Shdr	*lShdr;
	Elf32_Sym isyms;
	Elf32_Sym dyn_syms;
	int bytesread;
	mm_segment_t oldfs = get_fs();
	int symbol_count = 0;
	Elf32_Phdr	*temp_pPhdr = NULL;
	int err = 0;
	Elf32_Phdr	*aop_pPhdr = NULL;		/* Program Header */	
	Elf32_Shdr	*aop_pShdr = NULL;		/* Section Header */
	Elf32_Ehdr	aop_ehdr;				/* ELF Header */	
	
	struct file *aop_filp; /* File pointer to access file for ELF parsing*/

	/*
	* This member’s value gives the byte offset from the beginning of the 
	* file to the firstbyte in the section 
	*/
	uint32_t aop_symtab_offset = 0; 
	 /*Total no of enteries in Section header*/
	uint32_t aop_symtab_ent_no = 0;
	/*
	* This member holds a section header table index link, 
	* whose interpretation depends on the section type.
	*/
	int aop_symtab_str_link = 0;
	/*
	* This member’s value gives the byte offset from the beginning of the 
	* file to the firstbyte in the section*
	*/
	uint32_t aop_dyn_symtab_offset = 0;
	 /*Total no of enteries in dynamic symbol table*/
	uint32_t aop_dyn_symtab_ent_no = 0;
	/*
	* This member’s value gives the byte offset from the beginning of the 
	* file to the first byte in the section
	*/
	uint32_t aop_symstr_offset = 0; 
	  /*
	* This member holds a section header table index link, 
	* whose interpretation depends on the section type.
	*/
	int aop_dyn_symtab_str_link = 0;
	/*
	* This member’s value gives the byte offset from the beginning of the 
	* file to the firstbyte in the dynamic section string
	*/ 
	uint32_t aop_dyn_symstr_offset = 0;  
	 /*
	* This member’s value gives the byte offset from the beginning of the 
	* file to the firstbyte in the section header
	*/
	uint32_t aop_strtab_offset = 0;

	aop_printk("[%s] : [%d]\n", __FUNCTION__, __LINE__);

	// File Open
	/*aop_printk("%s file going to	read\n",filename);*/
	aop_filp = filp_open(filename, O_RDONLY | O_LARGEFILE, 0);

	if (IS_ERR(aop_filp)||(aop_filp==NULL)){
		aop_errk("error opening file\n");
		set_fs(oldfs);
		return 1;
	}

	if (aop_filp->f_op->read==NULL){
		aop_errk("read not allowed\n");
		err = -EIO;
		goto ERR;
	}

	if(!(aop_head_item[*list_num]= 
			(struct list_head*)vmalloc(sizeof(struct list_head)))){
		aop_errk("[aop_head_item] out of memory\n");
		err = -ENOMEM;
		goto ERR;
	}
	aop_printk("[%s] : aop_head_item[%d] :: %p\n", __FUNCTION__, *list_num,
				aop_head_item[*list_num]);
	printk("%s file loading....\n",filename);

	INIT_LIST_HEAD(aop_head_item[*list_num]);

	aop_filp->f_pos = 0;

	/*
	* Kernel segment override to datasegment and write it
	* to the accounting file.
	*/
	set_fs(KERNEL_DS);

	/*  ELF Header */
	if((bytesread = 
			aop_filp->f_op->read(aop_filp,(char *)&aop_ehdr,sizeof(Elf32_Ehdr),
			&aop_filp->f_pos)) < sizeof(Elf32_Ehdr)){
		aop_errk("aop_ehdr %d read bytes out of required %u\n", bytesread
				,sizeof(Elf32_Ehdr));
		err = -EIO;
		goto ERR;
	}

	plist->file_type = aop_ehdr.e_type;

	/* First of all, some simple consistency checks */
	if (memcmp(aop_ehdr.e_ident, ELFMAG, SELFMAG) != 0){
		aop_errk("Wrong ELF Header----!!!!\n");
		err = 1;
		goto ERR;
	}

	/* Program Header */
	if(!(aop_pPhdr = 
		(Elf32_Phdr*)kmalloc(sizeof(Elf32_Phdr)*aop_ehdr.e_phnum, GFP_KERNEL))){
		aop_errk("aop_ehdr.e_phnum out of memory\n");
		err = -ENOMEM;
		goto ERR;
	}

	aop_filp->f_pos = (loff_t)sizeof(Elf32_Ehdr);

	if((bytesread = aop_filp->f_op->read(aop_filp,(char *)aop_pPhdr,
			sizeof(Elf32_Phdr)*aop_ehdr.e_phnum,&aop_filp->f_pos)) 
				< sizeof(Elf32_Phdr)*aop_ehdr.e_phnum){
		aop_errk("aop_ehdr.e_phnum %d read bytes out of required %u\n", 
				bytesread, sizeof(Elf32_Phdr)*aop_ehdr.e_phnum);
		err = -EIO;
		goto ERR;
	}

	/* Section Header */
	if(!(aop_pShdr = (Elf32_Shdr*)kmalloc(sizeof(Elf32_Shdr)*aop_ehdr.e_shnum,
			GFP_KERNEL))){
		aop_errk("SectionHeader: Memory demand = %d---out of memory\n",
				(sizeof(Elf32_Shdr)*aop_ehdr.e_shnum));
		err = -ENOMEM;
		goto ERR;
	}

	if((aop_filp->f_pos = (loff_t)aop_ehdr.e_shoff) <= 0){
		err = -EIO;
		goto ERR;
	}

	if((bytesread = aop_filp->f_op->read(aop_filp,(char *)aop_pShdr,
			sizeof(Elf32_Shdr)*aop_ehdr.e_shnum,&aop_filp->f_pos)) 
				< sizeof(Elf32_Shdr)*aop_ehdr.e_shnum){
		aop_errk("SectionHeader: %d read bytes out of required %u\n", bytesread,
			sizeof(Elf32_Shdr)*aop_ehdr.e_shnum);
		err = -EIO;
		goto ERR;
	}

	lShdr = aop_pShdr;
	for(idx = 0; idx < aop_ehdr.e_shnum; idx++, lShdr++){
		if( idx == aop_ehdr.e_shstrndx) {
			aop_strtab_offset= lShdr->sh_offset;
		} else if (lShdr->sh_type == SHT_DYNSYM){
			aop_dyn_symtab_offset = lShdr->sh_offset;
			aop_dyn_symtab_ent_no= lShdr->sh_size / lShdr->sh_entsize;
			aop_dyn_symtab_str_link  = lShdr->sh_link;	
		} else if ((idx ==aop_dyn_symtab_str_link) && aop_dyn_symtab_str_link ){
			aop_dyn_symstr_offset = lShdr->sh_offset;
			plist->dyn_str_size = lShdr->sh_size;
			if(plist->dyn_str_size) {
				if(!(plist->dyn_sym_buff = (char*)
					vmalloc(sizeof(char)  * plist->dyn_str_size))){
					aop_printk("size = %d : dyn_sym_buff --out of memory\n",
							(sizeof(char)*plist->dyn_str_size));
					err = -ENOMEM;
					goto ERR;
				}	
			}
		} else if (   lShdr->sh_type == SHT_SYMTAB){

			aop_symtab_offset = lShdr->sh_offset;
			aop_symtab_ent_no= lShdr->sh_size / lShdr->sh_entsize;
			aop_symtab_str_link  = lShdr->sh_link;
		} else if ((idx ==aop_symtab_str_link ) && aop_symtab_str_link){
			aop_symstr_offset = lShdr->sh_offset;
			plist->sym_str_size = lShdr->sh_size;
			if(plist->sym_str_size) {
				if(!(plist->sym_buff = 
					(char*)vmalloc(sizeof(char)*plist->sym_str_size))){
					aop_errk("Symstrbuf out of memory\n");
					err = -ENOMEM;
					goto ERR;
				}
			}
		}
	}

	temp_pPhdr = aop_pPhdr;

	if(aop_ehdr.e_type != ET_DYN )	{		
		aop_printk("Type: %s\n",aop_get_file_type (aop_ehdr.e_type));		
		aop_printk("Address : 0x%08lx\n ",(unsigned long)aop_pPhdr->p_vaddr);	
	} else {
		for (idx = 0; idx < aop_ehdr.e_phnum; idx++, temp_pPhdr++){ 	
			if((temp_pPhdr->p_type == PT_LOAD) &&
					((temp_pPhdr->p_flags & PF_X ) == PF_X)) {		  
				aop_printk("Type: %s\n", aop_get_file_type (aop_ehdr.e_type));
				aop_printk("Virtual Address: 0x%08lx\n", 
					(unsigned long)aop_pPhdr->p_vaddr);
				plist->virtual_addr = temp_pPhdr->p_vaddr;		
			}	
		}	
	}

	if(!plist->sym_buff&& !plist->dyn_sym_buff){
		aop_errk("No DynSym and Sym Info present in ELF File : %s\n", filename);
		goto DONE;
	}

	if(plist->sym_buff){
		aop_printk("plist->sym_str_size = %d\n", plist->sym_str_size);
		symbol_count = 0;

		/*For Symbol String  Table */
		if((aop_filp->f_pos = (loff_t)aop_symstr_offset) <= 0){
			err = -EIO;
			goto _LOAD_DYSYM;
		}

		if((bytesread = aop_filp->f_op->read(aop_filp,plist->sym_buff,
			sizeof(char)*plist->sym_str_size,&aop_filp->f_pos)) 
				<sizeof(char)*plist->sym_str_size){
			aop_errk("SymStr: %d read bytes out of required %u\n", bytesread,
						sizeof(char)*plist->sym_str_size);
			printk("symstr is corrupted. Try to load dynsym data\n");
			goto _LOAD_DYSYM;
		}

		/* For Symbol Table */
		if((aop_filp->f_pos = (loff_t)aop_symtab_offset) <= 0){
			err = -EIO;
			goto ERR;
		}

		for(idx=0; idx<aop_symtab_ent_no; idx++) {
			if((bytesread = 
				aop_filp->f_op->read(aop_filp, (char *)&isyms,sizeof (Elf32_Sym),
				&aop_filp->f_pos)) < sizeof (Elf32_Sym)){
				aop_errk("SymTab: %d read bytes out of required %u\n", bytesread
						,sizeof (Elf32_Sym));
				err = -EIO;
				goto ERR;
			}
	
			if(ELF_ST_TYPE (isyms.st_info)==AOP_STT_FUNC){
				aop_sym_add(&isyms.st_value, &isyms.st_name,&isyms.st_info,
								list_num);
				symbol_count++;
			}
		}

		plist->dyn_str_size = 0;
		if(plist->dyn_sym_buff){
			vfree(plist->dyn_sym_buff);
			plist->dyn_sym_buff = NULL;
		}
	}

_LOAD_DYSYM:
	if(plist->dyn_sym_buff){
		aop_printk("plist->dyn_str_size = %d\n", plist->dyn_str_size);

		/*For dynamic string table*/
		if((aop_filp->f_pos = (loff_t)aop_dyn_symstr_offset) <= 0){
			err = -EIO;
			goto ERR;
		}

		if((bytesread = aop_filp->f_op->read(aop_filp, plist->dyn_sym_buff,
				sizeof(char)*plist->dyn_str_size, &aop_filp->f_pos)) 
					<sizeof(char)*plist->dyn_str_size){
				aop_errk("SectionHeader: %d read bytes out of required %u\n", 
					bytesread,
					sizeof(char)*plist->dyn_str_size);
			err = -EIO;
			goto ERR;
		}		

		/* For Dynamic Symbol Table */
		if((aop_filp->f_pos = (loff_t)aop_dyn_symtab_offset) <= 0){
			err = -EIO;
			goto ERR;
		}

		for(idx=0; idx<aop_dyn_symtab_ent_no; idx++){
			if((bytesread = aop_filp->f_op->read(aop_filp, (char *)&dyn_syms,
					sizeof (Elf32_Sym), &aop_filp->f_pos)) <sizeof (Elf32_Sym)){
				aop_errk("DynSymTab: %d read bytes out of required %u\n", 
						bytesread, sizeof (Elf32_Sym));

				err = -EIO;
				goto ERR;
			}
			if(ELF_ST_TYPE (dyn_syms.st_info)==AOP_STT_FUNC){
				aop_sym_add(&dyn_syms.st_value, &dyn_syms.st_name,
						&dyn_syms.st_info, list_num);
				symbol_count++;
			}
		}

		plist->sym_str_size =0;
		if(plist->sym_buff){
			vfree(plist->sym_buff);
			plist->sym_buff = NULL;
		}		
	}	

	plist->elf_symbol_count = symbol_count;
	goto DONE;

ERR:
	set_fs(oldfs);

	if(aop_pPhdr){
		kfree(aop_pPhdr);
		aop_pPhdr = NULL;
	}

	if(aop_pShdr){
		kfree(aop_pShdr);
		aop_pShdr = NULL;
	}

	/* when error occur while adding symbol info */
	aop_sym_delete_single_elf_db (list_num);

	plist->sym_str_size =0;
	plist->dyn_str_size = 0;
	if(plist->sym_buff){
		vfree(plist->sym_buff);
		plist->sym_buff = NULL;
	}
	if(plist->dyn_sym_buff){
		vfree(plist->dyn_sym_buff);
		plist->dyn_sym_buff = NULL;
	}	

	filp_close(aop_filp, NULL);
	return err;

DONE:
	printk("%s file loading is completed\n",filename);
	set_fs(oldfs);
	if(aop_pPhdr){
		kfree(aop_pPhdr);
		aop_pPhdr = NULL;
	}

	if(aop_pShdr){
		kfree(aop_pShdr);
		aop_pShdr = NULL;
	}	
	filp_close(aop_filp, NULL);
	return 0;
}

/*
 *   Find the Symbol info from Symbol database
 */
static int aop_sym_find(unsigned int* address, 
	aop_usb_elf_list_item *plist, char* pfunc_name)
{
	aop_kernel_symbol_item *t_item;
	aop_kernel_symbol_item *temp_item;
	struct list_head *pos;
	//int idx = 0;

	if(!plist->sym_str_size && !plist->dyn_str_size) {
		aop_printk("No DynSym and Sym Info present\n");
		return -EINVAL;
	}

	aop_printk("[%s] : [%d]\n", __FUNCTION__, __LINE__);
	if(!pfunc_name){
		printk("[%s] : pfunc_name is NULL\n", __FUNCTION__);	
		return -EINVAL ;	
	}
	if(!(temp_item = 
			(aop_kernel_symbol_item*)vmalloc(sizeof(aop_kernel_symbol_item)))){
		aop_errk("temp_item :: out of memory\n");
		return -ENOMEM;
	}

/* list_for_each() is a macro for a for loop. 
 * first parameter is used as the counter in for loop. in other words, inside the
 * loop it points to the current item's list_head.
 * second parameter is the pointer to the list. it is not manipulated by the macro.
 */
	list_for_each(pos, aop_head_item[plist->elf_entry_no]) {
/* at this point: pos->next points to the next item's 'list' variable and 
 * pos->prev points to the previous item's 'list' variable. Here item is 
 * of type struct aop_kernel_symbol_item. But we need to access the item itself not the 
 * variable 'list' in the item! macro list_entry() does just that. See "How
 * does this work?" below for an explanation of how this is done.
 */			
		aop_printk("[%s] : Enter in the Loop\n", __FUNCTION__);
		t_item = list_entry(pos, aop_kernel_symbol_item, sym_list);

 /* given a pointer to struct list_head, type of data structure it is part of,
  * and it's name (struct list_head's name in the data structure) it returns a
  * pointer to the data structure in which the pointer is part of.
  * For example, in the above line list_entry() will return a pointer to the
  * struct aop_kernel_symbol_item item it is embedded in!
  */
	  	if (t_item){
			if ( t_item->st_value > *address ){
				//aop_printk ("%6d: ", idx++);
#if IS_DEBUG_ON				
				aop_print_vma (temp_item->st_value, LONG_HEX);
#endif
				aop_printk (" %-7s", aop_get_symbol_type
					(ELF_ST_TYPE (temp_item->st_info)));
				if(plist->sym_str_size ){
					aop_printk (" %s \n", 
						AOP_SYM_NAME(temp_item,
						plist->sym_buff,
						plist->sym_str_size));
					snprintf(pfunc_name, AOP_MAX_ELF_FILE_NAME_LEN, "%s",
							AOP_SYM_NAME(temp_item,
						plist->sym_buff,
						plist->sym_str_size));
				}else{
					aop_printk (" %s \n", 
							AOP_DYN_SYM_NAME(temp_item,
								plist->dyn_sym_buff,
								plist->dyn_str_size ));
					snprintf(pfunc_name, AOP_MAX_ELF_FILE_NAME_LEN, "%s",
							AOP_DYN_SYM_NAME(temp_item,
								plist->dyn_sym_buff,
								plist->dyn_str_size ));
				}
				*address = temp_item->st_value;
				aop_printk("[pfunc_name]  :: %s\n", pfunc_name);
				break;
			}else if(t_item->st_value == *address){
				//aop_printk ("%6d: ", idx++);
#if IS_DEBUG_ON				
				aop_print_vma (t_item->st_value, LONG_HEX);
#endif

				aop_printk (" %-7s", aop_get_symbol_type
							(ELF_ST_TYPE (t_item->st_info)));
				aop_printk (" %-6s", aop_get_symbol_binding
							(ELF_ST_BIND (t_item->st_info)));
				if(plist->sym_str_size) {
					aop_printk (" %s \n", AOP_SYM_NAME(t_item,
						plist->sym_buff,
						plist->sym_str_size));
					snprintf(pfunc_name, AOP_MAX_ELF_FILE_NAME_LEN, "%s",
							AOP_SYM_NAME(t_item,
						plist->sym_buff,
						plist->sym_str_size));
				}else{
					aop_printk (" %s \n", 
						AOP_DYN_SYM_NAME(t_item,
								plist->dyn_sym_buff,
								plist->dyn_str_size ));
					snprintf(pfunc_name, AOP_MAX_ELF_FILE_NAME_LEN, "%s",
							AOP_DYN_SYM_NAME(t_item,
								plist->dyn_sym_buff,
								plist->dyn_str_size ));
				}
				*address = t_item->st_value;
				break;
			}else{
				temp_item->st_value = t_item->st_value;
				temp_item->st_info= t_item->st_info;			
				temp_item->st_name= t_item->st_name;		
			}
		}else	{
			printk("[%s] : t_item is NULL\n", __FUNCTION__);	
			vfree (temp_item);
			return -ENOMEM ;
		}
	}

	vfree (temp_item);
	return 0;
}

/*
 *   Listing all ELF USB files 
 */
static int aop_sym_list_db_stats(void)
{
	aop_usb_elf_list_item *plist = NULL;
	struct list_head *pos;	
	int idx = 0;

	aop_printk("[%s] : [%d]\n", __FUNCTION__, __LINE__);
	list_for_each(pos, &aop_usb_aop_head_item) {
		plist = list_entry(pos, aop_usb_elf_list_item, usb_elf_list);
		if(!plist){
			aop_errk("plist is NULL\n");	
			return -1;
		}			

		if (!idx){
			printk("Index\tUSB_PATH\tELF_Name\tSymbol_Count\n");
			printk("================================="
					"==============================\n");			
		}

		printk("%2d\t",++idx);
		printk("%s\t", plist->path_name);
		printk("%-7s\t\t", plist->elf_name);
		printk("%-8d\n", plist->elf_symbol_count);
	}

	if(!plist)
		printk("NO ELF Files Found in USB \n");

	return 0;
}

/*
 *   List all the Symbol info from Symbol database
 */
static int aop_sym_list_elf_db(void)
{
	aop_kernel_symbol_item *t_item;
	struct list_head *elf_pos, *q;
	aop_usb_elf_list_item *plist;
	struct list_head *usb_pos;	
	int list_num;
	int idx = 0;
	int index = 0;
	int choice;	

	aop_printk("[%s] : [%d]\n", __FUNCTION__, __LINE__);
	aop_sym_list_db_stats();

	printk("Select the File ==>");
	choice = debugd_get_event();
	if(choice == 0)
		return 0;

	printk("\nSymbol list using list_for_each_safe()\n");
	printk("   Num:    Value  Type    Bind   Name\n");

	list_for_each(usb_pos, &aop_usb_aop_head_item){
		plist = list_entry(usb_pos, aop_usb_elf_list_item, usb_elf_list);
		if(!plist){
			aop_errk("plist is NULL\n");
			return -ENOMEM;			
		}

		if(++index == choice) {		
			list_num = plist->elf_entry_no;

			aop_printk("%s", plist->elf_name);
			if(!plist->sym_str_size && !plist->dyn_str_size){
				printk("No DynSym and Sym Info present in ELF File : %s\n",
						plist->elf_name);
				return 0;
			}

			list_for_each_safe(elf_pos, q, aop_head_item[list_num]){
				t_item = list_entry(elf_pos, aop_kernel_symbol_item, sym_list);
		  		if(t_item)	{
					printk ("%6d: ", idx++);
					aop_print_vma (t_item->st_value, LONG_HEX);
					printk (" %-7s", aop_get_symbol_type
							(ELF_ST_TYPE (t_item->st_info)));
					printk (" %-6s", aop_get_symbol_binding
							(ELF_ST_BIND (t_item->st_info)));
						
					if(plist->sym_str_size){
						printk (" %s \n", AOP_SYM_NAME(t_item,
						plist->sym_buff,
						plist->sym_str_size));
					}else{
						printk (" %s \n", 
							AOP_DYN_SYM_NAME(t_item,
								plist->dyn_sym_buff,
								plist->dyn_str_size ));
					}
				} else {
					aop_errk("t_item is NULL\n");
					return -ENOMEM ;
				}
			}
		}
	}
	return 0;
}

/*
 *   Delete all ELF file from sym database
 */
static void aop_sym_db_delete(void)
{
	aop_usb_elf_list_item *plist;
	struct list_head *usb_pos, *q;

	aop_printk("[%s] : [%d]\n", __FUNCTION__, __LINE__);	
	
	list_for_each_safe(usb_pos, q, &aop_usb_aop_head_item){
		plist = list_entry(usb_pos, aop_usb_elf_list_item, usb_elf_list);
		if(!plist){
			aop_errk("plist is NULL\n");	
			return ;			
		}	

		list_del(usb_pos);
		if(plist->sym_buff){
			vfree(plist->sym_buff);
		}
		if(plist->dyn_sym_buff){
			vfree(plist->dyn_sym_buff);
		}
		vfree(plist);
	}

	return;
}

/*
 *   Symbol info related datbase deletion  from the link list 
 */
static void aop_sym_delete(void)
{
	aop_kernel_symbol_item *t_item;
	struct list_head *elf_pos, *q;
	aop_usb_elf_list_item *plist;
	struct list_head *usb_pos;		  
	int list_num;

	aop_printk("[%s] : [%d]\n", __FUNCTION__, __LINE__);
/* now let's be good and free the aop_kernel_symbol_item items. since we will be removing 
 * items off the list using list_del() we need to use a safer version of the list_for_each() 
 * macro aptly named list_for_each_safe(). Note that you MUST use this macro if the loop 
 * involves deletions of items (or moving items from one list to another).
 */	list_for_each(usb_pos, &aop_usb_aop_head_item) {
		plist = list_entry(usb_pos, aop_usb_elf_list_item, usb_elf_list);
		if(!plist){
			aop_errk("plist is NULL\n");
			return ;			
		}
		list_num = plist->elf_entry_no;
	
		list_for_each_safe(elf_pos, q, aop_head_item[list_num]){
			t_item = list_entry(elf_pos, aop_kernel_symbol_item, sym_list);
			if(!t_item){
				aop_errk("t_item is NULL\n");
				return ;			
			}			 
			list_del(elf_pos);
			kfree(t_item);
		}
	}

	aop_sym_db_delete();
	printk("Unloaded the ELF database \n");
	return;	
}

/*
 *   Delete ELF database for partcular ELF name matches 
 */
static int  aop_sym_delete_given_elf_db(const char* elf_name, int* index)
{
	aop_usb_elf_list_item *plist;
	struct list_head *usb_pos, *q;

	aop_printk("[%s] : [%d]\n", __FUNCTION__, __LINE__);	

	list_for_each_safe(usb_pos, q, &aop_usb_aop_head_item){
		plist = list_entry(usb_pos, aop_usb_elf_list_item, usb_elf_list);

		if(!plist){
			aop_errk("plist is NULL\n");	
			return -1;			
		}

		if(!strncmp(plist->elf_name, elf_name, strlen(elf_name))) {
			*index = plist->elf_entry_no;
			aop_printk("Elf_name = %s :: %s, List No: %d\n", plist->elf_name,
						elf_name, *index);
			aop_sym_delete_single_elf_db(index);

			if(plist->sym_buff){
				vfree(plist->sym_buff);
			}

			if(plist->dyn_sym_buff){
				vfree(plist->dyn_sym_buff);
			}

			list_del(usb_pos);
			vfree(plist);
			return 0;
		}
	}
	return -1;
}

/*
 *  Get total no of  ELF file in sym database
 */
static int aop_sym_db_no_of_elf_files(void)
{
	aop_usb_elf_list_item *plist = NULL;
	struct list_head *pos;	
	int idx = 0;

	aop_printk("[%s] : [%d]\n", __FUNCTION__, __LINE__);
	list_for_each(pos, &aop_usb_aop_head_item)
	{
		plist = list_entry(pos, aop_usb_elf_list_item, usb_elf_list);
		if(!plist){
			aop_errk("plist is NULL\n");	
			return -1;			
		}			
		idx++;
	}

	if(!plist)
		printk("NO ELF Files Found in USB \n");

	return idx;
}

/*
 *  compare w.r.t address  
 */
static int aop_sym_cmp_addr( const struct list_head *a, 
					const struct list_head *b )
{
	aop_kernel_symbol_item *a_item;
	aop_kernel_symbol_item *b_item;
	unsigned int first = 0;
	unsigned int second = 0;

	a_item = list_entry(a, aop_kernel_symbol_item, sym_list);
	if(!a_item){
		aop_errk("a_item is NULL\n");
		return 1;
	}	
	first = a_item->st_value;

	b_item = list_entry(b, aop_kernel_symbol_item, sym_list);
	if(!b_item){
		aop_errk("b_item is NULL\n");
		return 1;
	}	
	second= b_item->st_value;

	if ( first < second ){
		return -1;
	} else if ( first == second ) {
		return 0;
	} else {
		return 1;
	}
}

/*
 *  Link list  sorting in ascending order 
 */
void aop_list_sort(struct list_head *head, aop_cmp cmp) 
{ 
	struct list_head *p, *q, *e, *list, *tail, *oldhead; 
	int insize, nmerges, psize, qsize, i; 

	aop_printk("[%s] : [%d]\n", __FUNCTION__, __LINE__);
	 
    list = head->next; 
    list_del(head); 
    insize = 1; 
    for (;;) { 
        p = oldhead = list; 
        list = tail = NULL; 
        nmerges = 0; 

        while (p) { 
            nmerges++; 
            q = p; 
            psize = 0; 
            for (i = 0; i < insize; i++) { 
	            psize++; 
	            q = q->next == oldhead ? NULL : q->next; 
	            if (!q) 
					break; 
            } 

            qsize = insize; 
            while (psize > 0 || (qsize > 0 && q)) { 
                if (!psize) { 
                    e = q; 
                    q = q->next; 
                    qsize--; 
                    if (q == oldhead) 
                        q = NULL; 
                } else if (!qsize || !q) { 
                    e = p; 
                    p = p->next; 
                    psize--; 
                    if (p == oldhead) 
                        p = NULL; 
                } else if (cmp(p, q) <= 0) { 
                    e = p; 
                    p = p->next; 
                    psize--; 
                    if (p == oldhead) 
                        p = NULL; 
                } else { 
					e = q; 
                    q = q->next; 
                    qsize--; 
                    if (q == oldhead) 
                    	q = NULL; 
                } 
                if (tail) 
                	tail->next = e; 
                else 
                    list = e; 
                e->prev = tail; 
                tail = e; 
            } 
            p = q; 
        } 

        tail->next = list; 
        list->prev = tail; 

        if (nmerges <= 1) 
			break; 

        insize *= 2; 
    } 

    head->next = list; 
    head->prev = list->prev; 
    list->prev->next = head; 
    list->prev = head; 
}

/*
 *  Get the func name  after Search the symbol by 
 *  recieving filename and addres(program counter) 
 */
int aop_get_symbol(char* pfilename, 
			unsigned int *symbol_addr, char * pfunc_name)
{
	aop_usb_elf_list_item *plist;
	struct list_head *pos;
	int retval;
	
	aop_printk("[%s] : [%d]\n", __FUNCTION__, __LINE__);
	list_for_each(pos, &aop_usb_aop_head_item)
	{
		plist = list_entry(pos, aop_usb_elf_list_item, usb_elf_list);
		if(!plist){
			aop_errk("pList is NULL\n");	
			return -1;			
		}

		if(!strncmp(plist->elf_name, pfilename, strlen(pfilename))){
			aop_printk(("  Type:                              %s\n"),
					      aop_get_file_type (plist->file_type));
			aop_printk("Virtual Address : 0x%8.8lx\n ", 
							(unsigned long) plist->virtual_addr);
			*symbol_addr += plist->virtual_addr;			
			retval = aop_sym_find(symbol_addr, plist, pfunc_name);
			*symbol_addr -= plist->virtual_addr;
			return retval;
		}
	}

	aop_printk("[%s] : File NOT FOUND In DATABASE : NO SYMBOL\n", pfilename);
	return -1;
}

/*Give the ELF name without path name out of the full name*/
char * aop_base_elf_name(const char * file)
{
	const char *pStr = 0;
	int  sepPos = -1;
	int ii = 0;
	int bSpace = 1;

	if(!file){
		return NULL;
	}

	pStr = file;

	for(ii = 0; *pStr; ++ii, ++pStr){
		if(*pStr == '/'){
			sepPos = ii;
		}

		if(bSpace && *pStr != ' ' && *pStr != '\t'){
			bSpace = 0;
		}
	}

	if(ii == 0){
		BUG_ON(!bSpace);
		return NULL;
	}

	if(!bSpace){
		BUG_ON(!(file && sepPos + 1 <= (int) strlen(file))); // check right boundary
		return (char *) (file + (sepPos + 1));
	}else{
		return NULL;
	}
}

/* Used to load executables and share libraries of the given PID. 
 return 0: success, -1= fail */
int aop_sym_get_exe_n_so_libs(pid_t pid, 
		struct aop_fs_lib_paths* p_lib_paths)
{
	struct vm_area_struct * vma; 
	struct file * file;
	static char path_buf[AOP_SYM_MAX_SO_LIB_PATH_LEN];
	struct task_struct* task;

	if (!p_lib_paths) {
		printk("[ALERT] null lib path\n");
		return -1;
	}
	p_lib_paths->num_paths = 0;

	/* find the task struct of the given PID */
	task =  find_task_by_pid(pid);
	if(!task || !task->mm){
		printk("[ALERT] %s Thread", (task==NULL) ? "No":"Kernel");
		printk("\n");
		return -1;
	}

	aop_printk("-----------------------------------------\n");
	aop_printk("* Read exe & all shared lib on pid (%d)\n", task->pid);
	aop_printk("-----------------------------------------\n");

	/* take the mm sem*/
	down_read(&task->mm->mmap_sem);

	/* take the mmap start address */
	vma = task->mm->mmap;

	/* lopp thru all the mmap and collect the executables for the give task */
	while(vma) {
	    file = vma->vm_file;
	    if (file &&
			(vma->vm_flags & VM_READ) &&
			(vma->vm_flags & VM_EXEC) &&
			(!(vma->vm_flags & VM_WRITE)) &&
			(!(vma->vm_flags & VM_MAYSHARE))) {
				char* p;

				/* decode path & executables */
	            p = d_path(file->f_dentry, 
					file->f_vfsmnt, path_buf, AOP_SYM_MAX_SO_LIB_PATH_LEN);

				/* validate the return value */
	            if (!IS_ERR (p) ) {
					aop_printk("%08x %s\n", (unsigned int)vma->vm_flags, p);

					/* copy the executable to given array */
					snprintf (
						p_lib_paths->libPath[p_lib_paths->num_paths],
						AOP_SYM_MAX_SO_LIB_PATH_LEN, "%s", p );
					p_lib_paths->num_paths++;

					/* validate the max limit */
					if (p_lib_paths->num_paths>=AOP_SYM_MAX_SO_LIBS){
						aop_printk("Lib Array Full %d !!!!!\n", 
							p_lib_paths->num_paths );
						break;
					}
	            }
	    }

	    vma = vma->vm_next;
	}

	/* release the mm sem*/
	up_read(&task->mm->mmap_sem);
	aop_printk("--------------------------------------------\n\n");

	return 0;
}

/* load elf database from bianry of user specified Pid */
int aop_sym_update_db_by_pid(pid_t pid)
{
	aop_usb_elf_list_item *plist, tmp_plist;
	int i = 0;
	char* elf_name = NULL;
	struct aop_fs_lib_paths* p_lib_paths;
	int list_num = 0;
	int aop_list_num = 0;

	if(!(p_lib_paths = 
		(struct aop_fs_lib_paths*)vmalloc(sizeof(struct aop_fs_lib_paths)))){
		aop_errk("p_lib_paths out of memory\n");
		return -ENOMEM;
	}

	if ( aop_sym_get_exe_n_so_libs(pid, p_lib_paths) == -1 ){
		vfree (p_lib_paths);
		return 1;
	}

	if(aop_config_additive){
		aop_list_num= aop_sym_db_no_of_elf_files();				

	}else{
		aop_sym_delete();
	}

	for(i = 0; i < p_lib_paths->num_paths; i++, aop_list_num++)
	{
		elf_name = aop_base_elf_name(p_lib_paths->libPath[i]);
		if(!elf_name || elf_name < p_lib_paths->libPath[i]){
			aop_printk("Error in spliting filename(%s)...\n", 
					p_lib_paths->libPath[i]);
			continue;
		}

		memset(&tmp_plist, 0, sizeof(tmp_plist));

		if(aop_config_additive){
			if(aop_sym_delete_given_elf_db(elf_name, &list_num)){
				list_num = aop_list_num;
			}	

			if(aop_sym_read_elf(p_lib_paths->libPath[i], 
					&tmp_plist, &list_num)){
				aop_errk("Failed to Load Symbol\n");
				continue;
			}
		}else{
			if(aop_sym_read_elf(p_lib_paths->libPath[i], 
						&tmp_plist,&aop_list_num)){
				aop_errk("Failed to Load Symbol\n");
				continue;
			}
		}

		if(!(plist= 
			(aop_usb_elf_list_item*)vmalloc(sizeof(aop_usb_elf_list_item)))){
			aop_printk("[plist] out of memory\n");
			vfree (p_lib_paths);
			return -ENOMEM;
		}

		if(elf_name == p_lib_paths->libPath[i]){
			plist->path_name[0] = '\0';
		}else{
			elf_name[-1] = '\0'; /*We have guarenteed that elf_name is always 
							greater than starting of elf_path ptr*/
			snprintf(plist->path_name, AOP_SYM_MAX_SO_LIB_PATH_LEN, "%s", 
					p_lib_paths->libPath[i]);
		}
		snprintf(plist->elf_name,  AOP_MAX_ELF_FILE_NAME_LEN, "%s", elf_name);

		if(aop_config_additive){
			plist->elf_entry_no = list_num;
		}
		else{
			plist->elf_entry_no = aop_list_num;
		}

		plist->elf_symbol_count = tmp_plist.elf_symbol_count;
		plist->sym_buff = tmp_plist.sym_buff;
		plist->dyn_sym_buff = tmp_plist.dyn_sym_buff;
		plist->sym_str_size = tmp_plist.sym_str_size;
		plist->dyn_str_size = tmp_plist.dyn_str_size;
		plist->file_type = tmp_plist.file_type;
		plist->virtual_addr = tmp_plist.virtual_addr;

		aop_printk("[%s] Type:%s\n",
				__FUNCTION__, aop_get_file_type (plist->file_type));
		aop_printk("[%s] Virtual Address : 0x%8.8lx \n", 
				__FUNCTION__, (unsigned long) plist->virtual_addr);

		list_add_tail(&plist->usb_elf_list, &aop_usb_aop_head_item);
		if(aop_config_additive){
			if(plist->sym_str_size ||plist->dyn_str_size)
				aop_list_sort(aop_head_item[list_num], aop_sym_cmp_addr);
		}else{
			if(plist->sym_str_size ||plist->dyn_str_size)
				aop_list_sort(aop_head_item[aop_list_num],aop_sym_cmp_addr);
		}
	}			

	if (aop_sym_load_notification_func)
		aop_sym_load_notification_func(1);

	vfree (p_lib_paths);
	return 0;				
}

/* load elf database from binary of user specified Pid */
static void aop_sym_load_elf_by_pid(void)
{
	struct task_struct *g, *p;
	int do_unlock = 1;
	pid_t pid;

	/* load elf database from bianry of user specified Pid */
	printk("Load Symbols From Pid:\n");

#ifdef CONFIG_PREEMPT_RT
	if (!read_trylock(&tasklist_lock)) {
		printk("hm, tasklist_lock write-locked.\n");
		printk("ignoring ...\n");
		do_unlock = 0;
	}
#else
	read_lock(&tasklist_lock);
#endif

	printk("---------------------------\n");
	printk(" PID  Command Name\n");
	printk("---------------------------\n");
	do_each_thread(g, p) {
		/*
		 * reset the NMI-timeout, listing all files on a slow
		 * console might take alot of time:
		 */
		touch_nmi_watchdog();

		if (!p->mm)
			printk("%5d [%s]\n", p->pid, p->comm );
		else if (p->sibling.prev==&p->parent->children)
			printk("%5d %s\n", p->pid, p->comm );
	} while_each_thread(g, p);
	printk("---------------------------\n");

	if (do_unlock)
		read_unlock(&tasklist_lock);

	printk("\nEnter the pid to load elf db ==>");
	pid = debugd_get_event();
	printk("\n");

	aop_sym_update_db_by_pid(pid);
}

/* load elf database from USB */
static int aop_sym_update_db_from_usb(void)
{
	static struct aop_dir_list dir_list;
	static struct aop_usb_path usb_path_list;
	int i = 0;
	char buff[AOP_MAX_ELF_FILE_NAME_LEN]= {0,};
	aop_usb_elf_list_item *plist, tmp_plist;
	int index =0;
	int list_num = 0;
	int aop_list_num = 0;

	if(aop_config_additive){
		aop_list_num= aop_sym_db_no_of_elf_files();				

	}else{
		aop_sym_delete();
	}

	memset(&usb_path_list, 0, sizeof(usb_path_list));

	if(aop_usb_detect(&usb_path_list)){
		return -1;
	}

	for(index =0; (index < usb_path_list.num_usb) && (index < AOP_MAX_USB);
		index++) {
		memset(&dir_list, 0, sizeof(dir_list));
		snprintf(dir_list.path_dir, AOP_MAX_PATH_LEN,"%s/%s/", 
					usb_path_list.name[index], AOP_ELF_PATH);
		if(aop_dir_read(&dir_list)){
			printk("ERROR Reading the USB\n");
			continue;
		}

		for(i = 0; i < dir_list.num_files; i++, aop_list_num++){
			aop_printk(" %s \n",dir_list.dirent[i].d_name);

			if(dir_list.dirent[i].d_name[0] == '.'){
				continue;
			}

			snprintf(buff, AOP_MAX_ELF_FILE_NAME_LEN, "%s%s", 
				dir_list.path_dir, dir_list.dirent[i].d_name);

			memset(&tmp_plist, 0, sizeof(tmp_plist));

			if(aop_config_additive){
				if(aop_sym_delete_given_elf_db(dir_list.dirent[i].d_name,
						&list_num)){
					list_num = aop_list_num;
				}	

				if(aop_sym_read_elf(buff, &tmp_plist, &list_num)){
					aop_errk("Failed to Load Symbol\n");
					continue;
				}
			}else{
				if(aop_sym_read_elf(buff, &tmp_plist, &aop_list_num)){
					aop_errk("Failed to Load Symbol\n");
					continue;
				}
			}						

			if(!(plist= (aop_usb_elf_list_item*)
					vmalloc(sizeof(aop_usb_elf_list_item)))){
				printk("aop_usb_elf_list_item out of memory\n");
				return -ENOMEM;
			}

			snprintf(plist->path_name, AOP_MAX_PATH_LEN, "%s",
					usb_path_list.name[index]);
			snprintf(plist->elf_name,  AOP_MAX_ELF_FILE_NAME_LEN, "%s", 
					dir_list.dirent[i].d_name);

			if(aop_config_additive){
				plist->elf_entry_no = list_num;
			}else{
				plist->elf_entry_no = aop_list_num;
			}

			plist->elf_symbol_count = tmp_plist.elf_symbol_count;
			plist->sym_buff = tmp_plist.sym_buff;
			plist->dyn_sym_buff = tmp_plist.dyn_sym_buff;
			plist->sym_str_size = tmp_plist.sym_str_size;
			plist->dyn_str_size = tmp_plist.dyn_str_size;
			plist->file_type = tmp_plist.file_type;
			plist->virtual_addr = tmp_plist.virtual_addr;

	    	aop_printk("[%s] Type:%s\n",
					__FUNCTION__, aop_get_file_type (plist->file_type));
			aop_printk("[%s] Virtual Address : 0x%8.8lx \n", 
					__FUNCTION__, (unsigned long) plist->virtual_addr);

			list_add_tail(&plist->usb_elf_list, &aop_usb_aop_head_item);
			if(aop_config_additive){
				if(plist->sym_str_size ||plist->dyn_str_size)
					aop_list_sort(aop_head_item[list_num], aop_sym_cmp_addr);
			}else{
				if(plist->sym_str_size ||plist->dyn_str_size)
					aop_list_sort(aop_head_item[aop_list_num],aop_sym_cmp_addr);
			}
		}

		/* notify elf database load done to caller */
		if (aop_sym_load_notification_func)
			aop_sym_load_notification_func(1);

		break;
	}

	return 0;
}

static int 	aop_sym_list_usb_files ( void )
{
	static struct aop_dir_list dir_list;
	static struct aop_usb_path usb_path_list;
	int index =0;
	int i = 0;

	memset(&usb_path_list, 0, sizeof(usb_path_list));
	if(aop_usb_detect(&usb_path_list)){
		return -1;
	}

	for(index =0;(index < usb_path_list.num_usb) && (index < AOP_MAX_USB);
		index++) {
		memset(&dir_list, 0, sizeof(dir_list));
		snprintf(dir_list.path_dir, AOP_MAX_PATH_LEN,"%s/%s/", 
					usb_path_list.name[index], AOP_ELF_PATH);
		if(aop_dir_read(&dir_list)== -1){
			printk("ERROR Reading the USB\n");
			continue;
		}
		break;
	}

	if(dir_list.num_files){
		printk("There are %d files(s) found in %s\n", 
				dir_list.num_files, dir_list.path_dir);
		for(i = 0; i < dir_list.num_files; i++) {
			printk(" [%d] : %s \n", (i+1), dir_list.dirent[i].d_name);
		}
	}else
		printk("Files not found in aop_bin/\n");

	return 0;
}

#if IS_DEBUG_ON
static int aop_sym_search_symbol_test(void)
{
	int idx = 0;
	aop_usb_elf_list_item *plist;
	struct list_head *pos;		
	char buff[AOP_MAX_ELF_FILE_NAME_LEN]= {0,};	
	int choice = 0;
	unsigned int address = 0;

	idx = 0;
	aop_sym_list_db_stats();
	printk("Select the File ==>");
	choice = debugd_get_event();

	memset(buff, 0, sizeof(buff));

	list_for_each(pos, &aop_usb_aop_head_item)
	{
		plist = list_entry(pos, aop_usb_elf_list_item, usb_elf_list);
		if(!plist){
			printk("[%s] : pList is NULL\n", __FUNCTION__);	
			return -1;;			
		}

		if(idx++ == choice){
			printk("\nFile Selected = %s\n", plist->elf_name);
			printk("\nEnter the Addr (In Decimal) ==>");
			address = debugd_get_event();
			printk("\n");
	
			aop_sym_find(&address, &plist, buff);
			printk("Func Name :: [%s]\n", buff);
			break;
		}
	}

	return 0;
}
#endif /* IS_DEBUG_ON */

/* to register elf file load/unload notification function
this notification is used to reset the aop_sym_report database */
void aop_sym_register_oprofile_elf_load_notification_func ( 
			aop_symbol_load_notification func )
{
	if (func)
		aop_sym_load_notification_func = func;
}

/*
FUNCTION NAME	 	:	aop_sym_kdmenu
DESCRIPTION			:	main entry routine for the ELF Parseing utility
ARGUMENTS			:	option , File Name
RETURN VALUE	 	:	0 for success
AUTHOR			 	:	Gaurav Jindal
**********************************************/
int aop_sym_kdmenu(void)
{
	int operation = 0;
	static struct aop_usb_path usb_path_list;

#if IS_DEBUG_ON
	char buff[AOP_MAX_ELF_FILE_NAME_LEN]= {0,};
	int index =0;
	int choice = 0;
	unsigned int address = 0;
#endif /* IS_DEBUG_ON */

	aop_printk("Update ELF Database as = %d\n", aop_config_additive);
	
	printk ("Options are:\n"
			" 1 Load All ELF from USB (aop_bin/)\n"
			" 2 Unload ELF Database\n"
			" 3 List Database Stats\n"
			" 4 List USB (aop_bin/) files\n"
			" 5 Search USB Mount paths\n"
			" 6 Print symbol info (Internal Test)\n"
			" 7 Load symbols with pid\n" );
#if IS_DEBUG_ON
	printk (" 8 Get Function name(Internal Test)\n"
			" 9 Search Symbol (Internal Test)\n"
			" 10 Select USB Mount path\n");
#endif
		
	printk("\n[Adv Oprofile:USB] Option ==>  ");
	
	operation = debugd_get_event();
	printk("\n");

	switch(operation)
	{
		case 1:
		{
			if(aop_sym_update_db_from_usb())
				goto ERR;
		}					
		break;
		case 2:
		{
			aop_sym_delete();

			/* notify elf database is cleared */
			if (aop_sym_load_notification_func)
				aop_sym_load_notification_func(0);
		}
		break;
		case 3:
		{
			aop_sym_list_db_stats();
		}
		break;		
		case 4:
		{
			if(aop_sym_list_usb_files())
				goto ERR;
		}
		break;
		case 5:
		{
			memset(&usb_path_list, 0, sizeof(usb_path_list));
			aop_usb_detect(&usb_path_list);
			if(!usb_path_list.num_usb)
				printk("USB NOT Connected\n");
		}		
		break;
		case 6:
		{
			if(aop_sym_list_elf_db())
				goto ERR;
		}
		break;
		case 7:
			aop_sym_load_elf_by_pid ();
		break;

#if IS_DEBUG_ON
		case 8:
		{
			memset(buff, 0, sizeof(buff));
			printk("\nEnter the Addr (In Decimal) ==>");
			address = debugd_get_event();
			aop_get_symbol("exeDSP", &address, buff);
			printk("Func Name :: [%s]\n", buff);

		}
		break;		
		case 9:
		{
			if(aop_sym_search_symbol_test())
				goto ERR;
		}
		break;
		case 10:
		{
			printk(" USB MOUNT PATH(s): \n");

			memset(&usb_path_list, 0, sizeof(usb_path_list));
			aop_usb_detect(&usb_path_list);
			if(usb_path_list.num_usb){
				for(index =0; index < usb_path_list.num_usb; index++)
				{
					printk("[%d] : %s\n", index, usb_path_list.name[index]);
				}
				printk("Select the USB Path ==>");
				choice = debugd_get_event();
			}
			else
				printk("USB NOT Connected\n");
		}		
		break;
#endif
		default:	break;
	}

	return 0;

ERR:
	return -1;
}


