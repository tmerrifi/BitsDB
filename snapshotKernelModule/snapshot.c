

#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/path.h>
#include <linux/string.h>
#include <linux/rmap.h>
#include <linux/highmem.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>

MODULE_LICENSE("GPL");

#define SNAPSHOT_PREFIX "snapshot_"

#define SNAPSHOT_DEBUG Y

int debugging_each_pte_entry (pte_t * pte, unsigned long addr, unsigned long next, struct mm_walk * walker){
	
	unsigned long pfn = pte_pfn(*pte);
	struct page * page = pte_page(*pte);
	
	trace_printk(	"SNAP: debugging PTE: value: %lu, dirty: %d, write: %d, file: %d, %p, pfn: %lu, pfn_page: %p, %d, %p \n", 
					pte_val(*pte),
					pte_dirty(*pte),
					pte_write(*pte),
					pte_file(*pte),
					pte,
					pfn,
					pfn_to_page(pfn),
					PageAnon(page),
					page->mapping );
					
	return 0;
}

void debugging_pte ( struct vm_area_struct * vma ){
	
	struct mm_walk pt_walker;
	memset(&pt_walker, 0, sizeof(struct mm_walk));	//need to zero it out so all of the function ptrs are NULL
	pt_walker.pte_entry = debugging_each_pte_entry;
	pt_walker.mm = vma->vm_mm;
	
	walk_page_range(vma->vm_start, vma->vm_end, &pt_walker);	//calls a function outside this module that handles this
	trace_printk("vm flags, HUH??????? %d\n", ( (vma->vm_flags & (VM_WRITE|VM_SHARED)) == (VM_WRITE|VM_SHARED) ) );
}

int is_snapshot (struct vm_area_struct * vma, struct mm_struct * mm, struct file * f){
	if (f && f->f_path.dentry){
		#ifdef SNAPSHOT_DEBUG
		trace_printk(	"SNAP: is_snapshot: file name is %s, starts: %d\n", 
						f->f_path.dentry->d_name.name,
						strstarts(f->f_path.dentry->d_name.name, SNAPSHOT_PREFIX) );
		#endif
		return strstarts(f->f_path.dentry->d_name.name, SNAPSHOT_PREFIX);
	}
	else{
		return 0;	
	}
}

int is_snapshot_read_only (struct vm_area_struct * vma){ 
	int result = 0;
	if (vma && vma->vm_file && vma->vm_file->f_path.dentry){
		#ifdef SNAPSHOT_DEBUG
		trace_printk(	"SNAP: is_snapshot_read_only: file %p : write %d\n", 
						vma->vm_file,
						!(vma->vm_flags & VM_WRITE) );
		#endif
		result = ( vma->vm_file && is_snapshot(NULL, NULL, vma->vm_file) && !(vma->vm_flags & VM_WRITE) );
		if (is_snapshot(NULL, NULL, vma->vm_file) && !result){
			trace_printk("SNAP: I am the master (hopefully)\n");
			debugging_pte(vma);	
		}
		return result;
	}
	else{
		return 0;
	}
}

int is_snapshot_master (struct vm_area_struct * vma){
	
	#ifdef SNAPSHOT_DEBUG 
	trace_printk(	"SNAP: is_snapshot_read_only: file %p : write %lu\n", 
					vma->vm_file,
					(vma->vm_flags & VM_WRITE) );
	#endif
	
	return ( vma->vm_file && is_snapshot(NULL, NULL, vma->vm_file) && vma->vm_flags & VM_WRITE );		
}

struct vm_area_struct * get_master_vma (struct file * vm_file, struct vm_area_struct * vma_current){
		struct prio_tree_iter iter;
		struct address_space * addr_space;
		struct vm_area_struct * vma;

		if (vm_file && vm_file->f_path.dentry && vm_file->f_path.dentry->d_inode &&
			vm_file->f_path.dentry->d_inode->i_mapping)
		{
			addr_space = vm_file->f_path.dentry->d_inode->i_mapping;
			//now lets loop through the vma's associated with this file
			trace_printk("in get_master_vma\n");
	
  	      	vma_prio_tree_foreach(vma, &iter, &(addr_space->i_mmap), 0, ULONG_MAX){
				if ( (vma->vm_flags & VM_WRITE) ){
					trace_printk(" write flags....%lu...vma is %p\n", (vma->vm_flags & VM_WRITE), vma );
					return vma;
				}
  	      	}
		}
		
		return NULL;
}

pte_t * get_pte_entry_from_address(struct mm_struct * mm, unsigned long addr){

	pgd_t * pgd;
	pud_t *pud;
	pte_t * pte;
	pmd_t *pmd;

	pgd = pgd_offset(mm, addr);
	if (!pgd_none(*pgd) && !pgd_bad(*pgd)){	//we have a global directory
		trace_printk("pgd entry is .... %lu\n", pgd_val(*pgd));
		pud = pud_offset(pgd, addr);
		trace_printk("pud entry is .... %lu\n", pud_val(*pud));
		if (!pud_none(*pud) && !pud_bad(*pud)){
			pmd = pmd_offset(pud, addr);
			if (!pmd_none(*pmd) && !pmd_bad(*pmd)){
				pte = pte_offset_map(pmd, addr);
				return pte;
			}
		}
	}
	
	return NULL;
}

int each_pte_entry (pte_t * pte, unsigned long addr, unsigned long next, struct mm_walk * walker){
	struct page * page, * current_page;	
	unsigned long readonly_addr;
	//trace_printk("SNAP: in each_pte_entry\n");
	int page_mapped_result = 0;
	pte_t * dest_pte, tmp_ro_pte, tmp_master_pte;
	
	page = pte_page(*pte);
	trace_printk(	"SNAP: each_pte_entry, %lu, anonymous: %lu, ksm: %lu\n", 
					pte_val(*pte), 
					((unsigned long)page->mapping & PAGE_MAPPING_ANON),
					((unsigned long)page->mapping & PAGE_MAPPING_KSM));
	
	if (page){
		//is this page mapped into our address space?
		struct vm_area_struct * vma_read_only = (struct vm_area_struct *)walker->private;
		trace_printk("SNAP: the original vma: %p, the new thing: %p \n", vma_read_only, page->mapping);
		page_mapped_result = page_mapped_in_vma(page, vma_read_only);
		if (!page_mapped_result){
			readonly_addr = (page->index << PAGE_SHIFT) + vma_read_only->vm_start;	//get the new address
			trace_printk("SNAP: no, it's not mapped, index %lu, addr1: %lu addr2: %lu, %lu, %d\n", 
						page->index, 
						readonly_addr, 
						addr,
						(page->index << PAGE_SHIFT),
						PAGE_SHIFT);
			dest_pte = get_pte_entry_from_address( vma_read_only->vm_mm, readonly_addr);	
			current_page = pte_page(*dest_pte);	//getting the page struct for the pte we just grabbed
			if (dest_pte){
				trace_printk( "SNAP: the page is.... %p ... %d \n", page, page->_count.counter );
				get_page(page);												//increment the ref count for this page
				trace_printk( "SNAP: the page is.... %p ... %d \n", page, page->_count.counter );
				tmp_ro_pte = mk_pte(page, vma_read_only->vm_page_prot);		//create the new pte given a physical page (frame)
				pte_mkold(tmp_ro_pte);											//clear the accessed bit
				set_pte(dest_pte, tmp_ro_pte);									//now set the new pte
				tmp_master_pte = ptep_get_and_clear(walker->mm, addr, pte);
				
				trace_printk(	"SNAP: debugging PTE: value: %lu, dirty: %d, write: %d, present: %d, anonymous? : %d, anon: %d, %p, %p \n", 
					pte_val(tmp_master_pte),
					pte_dirty(tmp_master_pte),
					pte_write(tmp_master_pte),
					pte_present(tmp_master_pte),
					PageAnon(page),
					PageAnon(current_page),
					page,
					current_page );
				
				
				tmp_master_pte = pte_wrprotect(tmp_master_pte);				//we need to write protect the owner's pte again
				
				page->mapping = current_page->mapping; 						//need to set the mapping back so it's no longer anonymous
				
				trace_printk(	"SNAP: debugging PTE: value: %lu, dirty: %d, write: %d, present: %d, anonymous? : %d, anon: %d, %p, %p \n", 
					pte_val(tmp_master_pte),
					pte_dirty(tmp_master_pte),
					pte_write(tmp_master_pte),
					pte_present(tmp_master_pte),
					PageAnon(page),
					PageAnon(current_page),
					page,
					current_page);
				
				set_pte(pte, tmp_master_pte);
				
				trace_printk(	"SNAP: debugging PTE: value: %lu, dirty: %d, write: %d, present: %d, ptr %p, anonymous? : %d, anon: %d, %p, %p \n", 
					pte_val(*pte),
					pte_dirty(*pte),
					pte_write(*pte),
					pte_present(*pte),
					pte,
					PageAnon(page),
					PageAnon(current_page),
					page,
					current_page);
				
				pte_unmap(dest_pte);	
			}									
		}
		
	}
		//no?
			//is the pte not empty?
				//yes
					//switch the current pte to be read-only.
					//flush the TLB so parent process can no longer write to this page.
					//we need to copy the pte.
					//put the new pte into our page table.
					//increment the page ref count
	
	
	/*if (page && ((unsigned long)page->mapping & PAGE_MAPPING_ANON){
		
	}
	if (page){
		page_mapped_result = page_mapped_in_vma(page, 	
	}*/
	return 0;
}

void walk_master_vma_page_table(struct vm_area_struct * vma, struct vm_area_struct * read_only_vma){
	struct mm_walk pt_walker;
	memset(&pt_walker, 0, sizeof(struct mm_walk));	//need to zero it out so all of the function ptrs are NULL
	pt_walker.pte_entry = each_pte_entry;
	pt_walker.mm = vma->vm_mm;
	pt_walker.private = read_only_vma;
	
	walk_page_range(vma->vm_start, vma->vm_end, &pt_walker);	//calls a function outside this module that handles this
}

int get_snapshot (struct vm_area_struct * vma){
	
	struct vm_area_struct * master_vma;
	
	trace_printk("SNAP: get_snapshot \n");
	
	if (vma && vma->vm_mm && vma->vm_file){
		master_vma = get_master_vma(vma->vm_file, vma);
		trace_printk("SNAP: walking the page table for MASTER : %p\n", master_vma);
		if (master_vma){
			walk_master_vma_page_table(master_vma, vma);
			//flush_tlb_mm(vma->vm_mm);		//flush the TLB here!!!
			trace_printk("is master shared??? : %lu current??? %lu \n", 
			(master_vma->vm_flags & VM_SHARED),
			(vma->vm_flags & VM_SHARED));
		}
		//trace_printk("SNAP: walking the page table for READ-ONLY! : %p\n", vma);
		//walk_vma_page_table(vma);
	}
	
	return 0;
}

void myDebugFunction (struct vm_area_struct * vma, struct mm_struct * mm, struct file * f,  const unsigned long address){

	trace_printk("my debug function got hit!!!!\n");
	trace_printk("the start is : %lu\n", vma->vm_start);
	trace_printk("the end is : %lu, diff is %lu\n", vma->vm_end, (vma->vm_end - vma->vm_start));
	trace_printk("the name of the file is %s\n", f->f_path.dentry->d_name.name);
}

void unmapDebugFunction (struct vm_area_struct * vma, struct mm_struct * mm){
	/*walk through the page tables*/
	trace_printk("SNAP: in unmapDebug Function");
}

int init_module(void)
{
	printk(KERN_INFO "Hello world 1.\n");
 
	tim_debug_instance.tim_get_unmapped_area_debug=myDebugFunction;
	tim_debug_instance.tim_unmap_debug=unmapDebugFunction;
	
	mmap_snapshot_instance.is_snapshot = is_snapshot;
	mmap_snapshot_instance.is_snapshot_read_only = is_snapshot_read_only;
	mmap_snapshot_instance.is_snapshot_master = is_snapshot_master;
	mmap_snapshot_instance.get_snapshot = get_snapshot;
	
	return 0;
}

void cleanup_module(void)
{
	tim_debug_instance.tim_get_unmapped_area_debug=NULL;
	tim_debug_instance.tim_unmap_debug=NULL;
	
	mmap_snapshot_instance.is_snapshot = NULL;
	mmap_snapshot_instance.is_snapshot_read_only = NULL;
	mmap_snapshot_instance.is_snapshot_master = NULL;
	mmap_snapshot_instance.get_snapshot = NULL;
	
	printk(KERN_INFO "Goodbye world 1.\n");
}




