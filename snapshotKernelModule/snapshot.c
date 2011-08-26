

#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/path.h>
#include <linux/string.h>
#include <linux/rmap.h>
#include <linux/highmem.h>
#include <linux/pagemap.h>
#include <linux/list.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>

MODULE_LICENSE("GPL");

#define SNAPSHOT_PREFIX "snapshot_"

#define SNAPSHOT_DEBUG Y

struct snapshot_version_list * _snapshot_create_version_list();

/*a structure that defines a node in the pte list. Each version of the snapshot memory keeps a list
of pte values that have changed. A subscriber traverses the pte_list for each version that has changed
since the last snapshot and then changes it's page table to use the pte's in the list.*/
struct snapshot_pte_list{
	struct list_head list;
	pte_t * pte;	
	unsigned long addr;
};

/*this structure is a list of snapshot_pte_list objects. Each node in this list represents a version of the
snapshot*/
struct snapshot_version_list{
	struct list_head list;
	struct snapshot_pte_list * pte_list;
};

int debugging_each_pte_entry (pte_t * pte, unsigned long addr, unsigned long next, struct mm_walk * walker){
	
	unsigned long pfn = pte_pfn(*pte);
	struct page * page = pte_page(*pte);
	
	trace_printk(	"SNAP: debugging PTE: value: %lu, dirty: %d, write: %d, file: %d, %p, pfn: %lu, pfn_page: %p, %d, %p %p \n", 
					pte_val(*pte),
					pte_dirty(*pte),
					pte_write(*pte),
					pte_file(*pte),
					pte,
					pfn,
					pfn_to_page(pfn),
					PageAnon(page),
					page->mapping,
					page );
					
	/*printk(	"SNAP: debugging PTE: value: %lu, dirty: %d, write: %d, file: %d, %p, pfn: %lu, pfn_page: %p, %d, %p %p \n", 
					pte_val(*pte),
					pte_dirty(*pte),
					pte_write(*pte),
					pte_file(*pte),
					pte,
					pfn,
					pfn_to_page(pfn),
					PageAnon(page),
					page->mapping,
					page );*/
							
	return 0;
}

void debugging_pte ( struct vm_area_struct * vma ){
	
	struct mm_walk pt_walker;
	memset(&pt_walker, 0, sizeof(struct mm_walk));	//need to zero it out so all of the function ptrs are NULL
	pt_walker.pte_entry = debugging_each_pte_entry;
	pt_walker.mm = vma->vm_mm;
	
	trace_printk("SNAP: vma info: start: %08x\n", vma->vm_start);
	
	walk_page_range(vma->vm_start, vma->vm_end, &pt_walker);	//calls a function outside this module that handles this
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
	struct page * page_test;
	
	if (vma && vma->vm_file && vma->vm_file->f_path.dentry){
		#ifdef SNAPSHOT_DEBUG
		trace_printk(	"SNAP: is_snapshot_read_only: file %p : write %d : vma is %p\n", 
						vma->vm_file,
						!(vma->vm_flags & VM_WRITE),
						vma );
		#endif
		result = ( vma->vm_file && is_snapshot(NULL, NULL, vma->vm_file) && !(vma->vm_flags & VM_WRITE) );
		if (is_snapshot(NULL, NULL, vma->vm_file) && !result){
			trace_printk("SNAP: I am the master (hopefully)\n");
			printk(KERN_INFO "Master msync with SetPageDirty\n");

			if (vma->vm_file->f_path.dentry->d_inode->i_mapping){
				page_test = radix_tree_lookup(&vma->vm_file->f_path.dentry->d_inode->i_mapping->page_tree, 0);
				if (page_test){
					trace_printk("page is 0 : %p, mapping %p file %p\n", page_test, vma->vm_file->f_path.dentry->d_inode->i_mapping, vma->vm_file);
				}
				
				page_test = radix_tree_lookup(&vma->vm_file->f_path.dentry->d_inode->i_mapping->page_tree, 1);
				if (page_test){
					trace_printk("page is 1 : %p mapping %p file %p\n", page_test, vma->vm_file->f_path.dentry->d_inode->i_mapping, vma->vm_file);
				}
				
				page_test = radix_tree_lookup(&vma->vm_file->f_path.dentry->d_inode->i_mapping->page_tree, 2);
				if (page_test){	
					trace_printk("page is 2 : %p  mapping %p file %p\n", page_test, vma->vm_file->f_path.dentry->d_inode->i_mapping, vma->vm_file);	
				}
			}
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
	trace_printk(	"SNAP: is_snapshot_read_only: file %p : write %lu, vma %p\n", 
					vma->vm_file,
					(vma->vm_flags & VM_WRITE),
					vma );
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
	
			trace_printk("the current vma is %p, the mapping is %p\n", vma_current, &vma_current->vm_file->f_mapping->i_mmap);
  	      	vma_prio_tree_foreach(vma, &iter, &vma_current->vm_file->f_mapping->i_mmap, 0, ULONG_MAX){
  	      		trace_printk("FOUND SOME VMA %p, made it in %d\n", vma, (vma->vm_flags & VM_WRITE)  );
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
	
	if (!pgd){
		trace_printk("pgd error\n");
		goto error;
	}
	pud = pud_alloc(mm, pgd, addr);
	
	if (!pud){
		trace_printk("pud error\n");
		goto error;
	}
	
	pmd = pmd_alloc(mm, pud, addr);
	
	if (!pmd){
		trace_printk("pmd error\n");
		goto error;	
	}
	
	//is there a pte already?
	
	//pte = pte_offset_map(pmd, addr);
	
	//trace_printk("the pmd is %p, pmd_present is %d\n", pmd, pmd_present(*pmd));
	//return NULL;
	
	pte = pte_alloc_map(mm, pmd, addr);
	
	//return NULL;
	
	if (!pte){
		trace_printk("pte error\n");
		goto error;
	}
	
	return pte;
	
	error:
		trace_printk("an error occured in get_pte_entry_from_address\n");
		return NULL;

}

int copy_pte_entry (pte_t * pte, unsigned long addr, struct vm_area_struct * vma_read_only, struct mm_struct * mm){
	struct page * page, * current_page, * page_test;	
	unsigned long readonly_addr;
	//trace_printk("SNAP: in each_pte_entry\n");
	int page_mapped_result = 0;
	
	if (pte_val(*pte) == 0){
		//trace_printk("nothing in the pte, just return\n");
		return 0;	
	}
	pte_t * dest_pte, tmp_ro_pte, tmp_master_pte;
	page = pte_page(*pte);
	if (page){
		//is this page mapped into our address space?
		page_mapped_result = page_mapped_in_vma(page, vma_read_only);
		if (!page_mapped_result){
			readonly_addr = (page->index << PAGE_SHIFT) + vma_read_only->vm_start;	//get the new address
			trace_printk("COPY PTE: vma_read_only: %p, vma_read_only->vm_start: %08x, readonly_addr %08x, mm %p\n", 
						vma_read_only, vma_read_only->vm_start, readonly_addr, vma_read_only->vm_mm);
			dest_pte = get_pte_entry_from_address( vma_read_only->vm_mm, readonly_addr);
			current_page = pte_page(*dest_pte);	//getting the page struct for the pte we just grabbed
			if (dest_pte){
				//increment the ref count for this page
				get_page(page);												
				//create the new pte given a physical page (frame)
				tmp_ro_pte = mk_pte(page, vma_read_only->vm_page_prot);
				//clear the accessed bit
				pte_mkold(tmp_ro_pte);
				//now set the new pte
				set_pte(dest_pte, tmp_ro_pte);
				tmp_master_pte = ptep_get_and_clear(mm, addr, pte);
				//we need to write protect the owner's pte again
				tmp_master_pte = pte_wrprotect(tmp_master_pte);	
				//set the mapping back so it's no longer anonymous
				page->mapping = vma_read_only->vm_file->f_path.dentry->d_inode->i_mapping;
				set_pte(pte, tmp_master_pte);
				pte_unmap(dest_pte);	
				//Now we need to mark the page in the radix tree as dirty so that the 
				//file system code will write it out to disk
				spin_lock_irq(&(page->mapping->tree_lock));
                SetPageDirty(page);
                //look to see if this guy is in the radix tree
                page_test = radix_tree_lookup(&vma_read_only->vm_file->f_path.dentry->d_inode->i_mapping->page_tree, page->index);
                if (page_test != page){
	                trace_printk("just looked up the page %p, %d at mapping %p file %p, page test is page? %d\n", 
	                				page_test, 
	                				page_test->index,
	                				vma_read_only->vm_file->f_path.dentry->d_inode->i_mapping,
	                				vma_read_only->vm_file,
	                				page_test == page);
	                if (page_test){
	                	radix_tree_delete(&vma_read_only->vm_file->f_path.dentry->d_inode->i_mapping->page_tree, page_test->index);
						page_test->mapping = NULL;
						vma_read_only->vm_file->f_path.dentry->d_inode->i_mapping->nrpages--;
	                }
	        		spin_unlock_irq(&page->mapping->tree_lock);
	               	lock_page(page);
	               	add_to_page_cache_locked(page, vma_read_only->vm_file->f_path.dentry->d_inode->i_mapping, page->index, GFP_KERNEL);
	               	radix_tree_tag_set(&vma_read_only->vm_file->f_path.dentry->d_inode->i_mapping->page_tree, page->index, PAGECACHE_TAG_DIRTY);
	               	unlock_page(page);
                }
                else{
                	spin_unlock_irq(&page->mapping->tree_lock);	
                }
			}							
		}
	}
	return 0;
	
}

int each_pte_entry (pte_t * pte, unsigned long addr, unsigned long next, struct mm_walk * walker){
	return copy_pte_entry (pte, addr, (struct vm_area_struct *)walker->private, walker->mm);
}

void walk_master_vma_page_table(struct vm_area_struct * vma, struct vm_area_struct * read_only_vma){
	struct mm_walk pt_walker;
	memset(&pt_walker, 0, sizeof(struct mm_walk));	//need to zero it out so all of the function ptrs are NULL
	pt_walker.pte_entry = each_pte_entry;
	pt_walker.mm = vma->vm_mm;
	pt_walker.private = read_only_vma;
	
	walk_page_range(vma->vm_start, vma->vm_end, &pt_walker);	//calls a function outside this module that handles this
}

void walk_page_table(struct vm_area_struct * vma){
	struct mm_walk pt_walker;
	memset(&pt_walker, 0, sizeof(struct mm_walk));	//need to zero it out so all of the function ptrs are NULL
	pt_walker.pte_entry = debugging_each_pte_entry;
	pt_walker.mm = vma->vm_mm;
	
	walk_page_range(vma->vm_start, vma->vm_end, &pt_walker);	//calls a function outside this module that handles this
}

int get_snapshot (struct vm_area_struct * vma){
	struct vm_area_struct * master_vma;
	/*for iterating through the list*/
	struct list_head * pos, * pos_outer;
	/*for storing pte values from list*/
	struct snapshot_pte_list * tmp_pte_list;
	struct snapshot_version_list * latest_version_entry, * new_version_entry;
	
	trace_printk("SNAP: get_snapshot \n");
	if (vma && vma->vm_mm && vma->vm_file){
		master_vma = get_master_vma(vma->vm_file, vma);
		trace_printk("file is %p, the vma mapping is %p, vma is %p\n", vma->vm_file, vma->vm_file->f_mapping->i_mmap, vma);
		if (master_vma){
			if (master_vma->snapshot_pte_list){
				struct snapshot_version_list * version_list = (struct snapshot_version_list *)master_vma->snapshot_pte_list;
				//get latest version list
				if (version_list && version_list->list.prev){
					//we need traverse the list of version lists
					list_for_each_prev(pos_outer, &version_list->list){
						latest_version_entry = list_entry( pos_outer, struct snapshot_version_list, list);
						if (latest_version_entry && latest_version_entry->pte_list){
							list_for_each(pos, &latest_version_entry->pte_list->list){
								tmp_pte_list = list_entry(pos, struct snapshot_pte_list, list);
								if (tmp_pte_list){
									trace_printk("traversing the pte list: %lu, %p %p, %08x\n", pte_val(*tmp_pte_list->pte), tmp_pte_list, tmp_pte_list->pte, tmp_pte_list->addr);
									copy_pte_entry (tmp_pte_list->pte, tmp_pte_list->addr, vma, master_vma->vm_mm);
								}
							}
						}
					}
					/*we need to add a new version list now*/
					new_version_entry = kmalloc(sizeof(struct snapshot_version_list), GFP_KERNEL);
					INIT_LIST_HEAD(&new_version_entry->list);
					new_version_entry->pte_list = NULL;
					/*add the entry to the list*/
					list_add(&new_version_entry->list, &version_list->list);
				}
			}
		}
	}
	
	return 0;
}

void myDebugFunction (struct vm_area_struct * vma, struct mm_struct * mm, struct file * f,  const unsigned long address){

	trace_printk("my debug function got hit!!!!\n");
	trace_printk("the file is %p, the vma is %p\n", f, vma);
}

void unmapDebugFunction (struct vm_area_struct * vma, struct mm_struct * mm){
	
	pgd_t * pgd;
	
	/*walk through the page tables*/
	trace_printk("SNAP: in unmapDebug Function");
	//debugging_pte ( vma );
	pgd = pgd_offset(mm, vma->vm_start);
	trace_printk("SNAP: pgd val: %lu\n", pgd_val(*pgd));
}

struct snapshot_version_list * _snapshot_create_version_list(){
	struct snapshot_version_list * version_list = kmalloc(sizeof(struct snapshot_version_list), GFP_KERNEL);
	INIT_LIST_HEAD(&version_list->list);
	/*now we need to add our first entry*/
	struct snapshot_version_list * version_entry = kmalloc(sizeof(struct snapshot_version_list), GFP_KERNEL);
	INIT_LIST_HEAD(&version_entry->list);
	version_entry->pte_list = NULL;
	/*add the entry to the list*/
	list_add(&version_entry->list, &version_list->list);
	return version_list;
}

struct snapshot_pte_list * _snapshot_create_pte_list(){
	struct snapshot_pte_list * pte_list = kmalloc(sizeof(struct snapshot_pte_list), GFP_KERNEL);
	INIT_LIST_HEAD(&pte_list->list);
	return pte_list;
}

int init_snapshot (struct vm_area_struct * vma){
	trace_printk("SNAP: in init_snapshot 2\n");
	/*do we need to initialize the list?*/
	/*create a new version list node*/
	struct snapshot_version_list * version_list = _snapshot_create_version_list();
	/*setting the ptr on the vma to point at our new list*/
	vma->snapshot_pte_list = version_list;
	trace_printk("file is %p, the vma mapping is %p, vma is %p\n", vma->vm_file, vma->vm_file->f_mapping->i_mmap, vma);
}

int do_snapshot_add_pte (struct vm_area_struct * vma, pte_t * orig_pte, pte_t * new_pte, unsigned long address){
	trace_printk("SNAP: in do_snapshot_add_pte\n");
	/*this should never happen */
	if (!vma->snapshot_pte_list){
		trace_printk("SNAP: creating new snapshot pte list\n");
		/*create a new version list node*/
		struct snapshot_version_list * tmp_version_list = _snapshot_create_version_list();
		/*setting the ptr on the vma to point at our new list*/
		vma->snapshot_pte_list = tmp_version_list;
	}
	/*get version list from vma*/
	struct snapshot_version_list * version_list = vma->snapshot_pte_list;
	/*get the latest version list, AKA the tail of the list*/
	struct snapshot_version_list * latest_version_entry = list_entry( version_list->list.prev, struct snapshot_version_list, list);
	
	/*if we are the first page fault, we have to create the pte list*/
	trace_printk("SNAP: version_list->list.prev %p %p\n", version_list->list.prev,  version_list);
	if (!latest_version_entry->pte_list){
		latest_version_entry->pte_list = _snapshot_create_pte_list();
	}
	
	/*DEBUGGING!!!!*/
	int version_list_size=0;
	struct list_head * pos;
	list_for_each(pos, &version_list->list){
		++version_list_size;
	}
	trace_printk("version list size is %d\n",version_list_size);
	
	/*create the new pte entry*/
	struct snapshot_pte_list * pte_list_entry = kmalloc(sizeof(struct snapshot_pte_list), GFP_KERNEL);
	INIT_LIST_HEAD(&pte_list_entry->list);
	pte_list_entry->pte = new_pte;
	pte_list_entry->addr = address;
	trace_printk("new_pte pte is : %p, new_pte val is : %lu, orig pte: %p, orig pte val : %lu, pte_list_entry %p, address %08x\n", 
				new_pte, pte_val(*new_pte), orig_pte, pte_val(*orig_pte),pte_list_entry, address );
	/*now we need to add the pte to the list */
	list_add(&pte_list_entry->list, &latest_version_entry->pte_list->list);

	/*DEBUGGING!!!!*/
	int pte_list_size=0;
	list_for_each(pos, &latest_version_entry->pte_list->list){
		++pte_list_size;
	}
	trace_printk("pte list size is %d\n",pte_list_size);


	struct prio_tree_iter iter;
	struct vm_area_struct * tmp_vma;
	trace_printk("the current vma is %p, the mapping is %p\n", vma, &vma->vm_file->f_mapping->i_mmap);
	vma_prio_tree_foreach(tmp_vma, &iter, &vma->vm_file->f_mapping->i_mmap, 0, ULONG_MAX){
		trace_printk("got vma: %p\n", tmp_vma);
	}
}

int init_module(void)
{
	printk(KERN_INFO "Hello world 1.\n");
 
	tim_debug_instance.tim_get_unmapped_area_debug=myDebugFunction;
	//tim_debug_instance.tim_unmap_debug=unmapDebugFunction;
	
	mmap_snapshot_instance.is_snapshot = is_snapshot;
	mmap_snapshot_instance.is_snapshot_read_only = is_snapshot_read_only;
	mmap_snapshot_instance.is_snapshot_master = is_snapshot_master;
	mmap_snapshot_instance.get_snapshot = get_snapshot;
	mmap_snapshot_instance.init_snapshot = init_snapshot;
	mmap_snapshot_instance.do_snapshot_add_pte = do_snapshot_add_pte;
	
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




