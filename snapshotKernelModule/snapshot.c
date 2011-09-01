

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
	int ref_count;
	struct snapshot_pte_list * pte_list;
};

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
		return ( vma->vm_file && is_snapshot(NULL, NULL, vma->vm_file) && !(vma->vm_flags & VM_WRITE) );
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
  	      	vma_prio_tree_foreach(vma, &iter, &vma_current->vm_file->f_mapping->i_mmap, 0, ULONG_MAX){
				if ( (vma->vm_flags & VM_WRITE) ){
					return vma;
				}
  	      	}
		}
		return NULL;
}

void print_pte_debug_info(pte_t * pte){
	trace_printk(	"SNAP: debugging PTE: value: %lu, dirty: %d, write: %d, file: %d, %p\n", 
					pte_val(*pte),
					pte_dirty(*pte),
					pte_write(*pte),
					pte_file(*pte),
					pte);
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
	pte = pte_alloc_map(mm, pmd, addr);
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
	int page_mapped_result = 0;
	pte_t * dest_pte, tmp_ro_pte, tmp_master_pte;
	page = pte_page(*pte);
	if (page){
		//is this page mapped into our address space?
		page_mapped_result = page_mapped_in_vma(page, vma_read_only);
		if (!page_mapped_result){
			readonly_addr = (page->index << PAGE_SHIFT) + vma_read_only->vm_start;	//get the new address
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

/*Given a element in the list (target) and the list itself, is this the latest version in the list? If so, then it is 
the uncommitted stuff*/
int staged_version_list(struct list_head * target, struct list_head * ls){
	return (ls && target == ls->next);
}

/*Given a element in the list (target) and the list itself, is this the latest COMMITTED version in the list?*/
int last_committed_version_list(struct list_head * target, struct list_head * ls){
	return (target && ls && staged_version_list(target->next, ls) );
}

void merge_and_remove_list(struct list_head * target, struct list_head * ls){
	struct snapshot_version_list * target_version, * next_version;
	struct list_head * pte_list_target, * pte_list_next, * tmp_for_safe;
	struct snapshot_pte_list * pte_entry, * pte_entry_next, * new_pte_entry; 
	//used for seaching the lists
	int exists;

	//grab the version infront of you (if it exists)		
	struct list_head * next_list = target->prev;
	trace_printk("\n\n\n\nin merge\n");
	if (!staged_version_list(next_list, ls)){
		//get the pte list for this guy
		target_version = list_entry( target, struct snapshot_version_list, list);
		next_version = list_entry( next_list, struct snapshot_version_list, list);
		if (target_version->pte_list && next_version->pte_list){
			//traverse the pte list...
			list_for_each_safe(pte_list_target, tmp_for_safe, &target_version->pte_list->list){
				exists=0;
				pte_entry = list_entry( pte_list_target, struct snapshot_pte_list, list);
				//look for this addr in the next list
				list_for_each(pte_list_next, &next_version->pte_list->list){
					pte_entry_next = list_entry( pte_list_next, struct snapshot_pte_list, list);
					//TODO: fix this for HUGE PAGES
					if ((pte_entry->addr & PAGE_MASK) == (pte_entry_next->addr & PAGE_MASK)){
						exists=1;
						trace_printk("it already exists\n");	
						break;
					}
				}
				if (!exists){
					//trace_printk("adding it\n");
					
					//trace_printk("deleting %08lx and pte %p pte_val %lu\n", pte_entry->addr, pte_entry->pte, pte_val(*pte_entry->pte));
					//trace_printk("We are deleting this from list %p and the entry is %p and the list element is %p\n",  
					//				&target_version->pte_list->list, pte_entry,pte_list_target);
					list_del(pte_list_target);
					/*something is up here*/
					//INIT_LIST_HEAD(pte_list_target);
					list_add(pte_list_target, &next_version->pte_list->list);
				}
			}
			//now remove our list
			list_del(target);
		}
	}
}

int get_snapshot (struct vm_area_struct * vma){
	struct vm_area_struct * master_vma;
	/*for iterating through the list*/
	struct list_head * pos, * pos_outer, * ls, * tmp, * new_list;
	/*for storing pte values from list*/
	struct snapshot_pte_list * tmp_pte_list;
	struct snapshot_version_list * latest_version_entry, * new_version_entry, * tmp_version_entry;
	int set_new_list = 0;
	
	trace_printk("\n\n\n\nSNAP: get_snapshot \n");
	if (vma && vma->vm_mm && vma->vm_file){
		master_vma = get_master_vma(vma->vm_file, vma);
		trace_printk("file is %p, the vma mapping is %p, vma is %p\n", vma->vm_file, vma->vm_file->f_mapping->i_mmap, vma);
		if (master_vma){
			if (master_vma->snapshot_pte_list){
				//get the latest version list
				struct snapshot_version_list * version_list = (struct snapshot_version_list *)master_vma->snapshot_pte_list;
				//only go in here if there is more than one element in the list (prev != next)
				if (version_list && version_list->list.prev != version_list->list.next){
					//if the subscriber's vma has a previous ptr into the list, use that. If not, just use the entire list
					if (vma->snapshot_pte_list){
						ls = (struct list_head *)vma->snapshot_pte_list;
						//if we are already the latest committed version, no reason to go through all of this
						if (last_committed_version_list(ls,&version_list->list)){
							return 1;
						}
					}
					else{
						//no previous ptr to list, just use the whole thing
						ls = &version_list->list;
					}
					
					list_for_each_prev(pos_outer, ls){
						if (staged_version_list(pos_outer,&version_list->list)){
							break;
						}
						latest_version_entry = list_entry( pos_outer, struct snapshot_version_list, list);
						if (latest_version_entry && latest_version_entry->pte_list){
							//now traverse the pte list to see what has changed.
							list_for_each(pos, &latest_version_entry->pte_list->list){
								//get the pte entry
								tmp_pte_list = list_entry(pos, struct snapshot_pte_list, list);
								if (tmp_pte_list){
									trace_printk("\ntraversing the pte list: %lu, %p %p, %08x\n", 
													pte_val(*tmp_pte_list->pte), tmp_pte_list, 
													tmp_pte_list->pte, tmp_pte_list->addr);
									trace_printk("the entry is %p, the list is %p and the entry's list is %p\n", 
													tmp_pte_list, &latest_version_entry->pte_list->list, pos);
									//now actually perform the copy
									copy_pte_entry (tmp_pte_list->pte, tmp_pte_list->addr, vma, master_vma->vm_mm);
								}
							}
							new_list = pos_outer;
						}
					}
					trace_printk("huh? %p, %d, %p\n", vma->snapshot_pte_list, new_list != vma->snapshot_pte_list, new_list);
					//deal with older list that we are no longer using
					if (vma->snapshot_pte_list && new_list != vma->snapshot_pte_list){
						//get the old list entry
						tmp_version_entry = list_entry( (struct list_head *)vma->snapshot_pte_list, struct snapshot_version_list, list);
						trace_printk("the ref count is %d\n", tmp_version_entry->ref_count);
						if (tmp_version_entry->ref_count == 1){
							//we are able to merge this one
							merge_and_remove_list(vma->snapshot_pte_list, &version_list->list);
						}
					}
					vma->snapshot_pte_list = new_list;
					latest_version_entry->ref_count +=1;
				}
			}
		}
	}
	return 0;
}

/*This is the main commit function for owners. We need to traverse the list and make the
modified pages COW. Here we also update the page cache*/
void snapshot_commit(struct vm_area_struct * vma){
	
	struct list_head * pos;
	struct snapshot_version_list * latest_version_entry, * new_version_entry;
	struct snapshot_pte_list * pte_entry;
	pte_t tmp_master_pte, * debug_pte;
	struct page * page;
	struct page * page_test;
	struct address_space * mapping;
	struct radix_tree_root *root;
	
	if (vma && vma->snapshot_pte_list){
		
		trace_printk("IN COMMIT \n");
		struct snapshot_version_list * version_list = (struct snapshot_version_list *)vma->snapshot_pte_list;
		if (version_list && version_list->list.prev){
			//get the latest list of modifications that haven't been committed (at the head of the list)
			latest_version_entry = list_entry(version_list->list.next, struct snapshot_version_list, list);
			//now, traverse the list
			trace_printk("IN COMMIT, before the list \n");
			if (latest_version_entry && latest_version_entry->pte_list){
				trace_printk("IN COMMIT, do we have a list? \n");
				list_for_each(pos, &latest_version_entry->pte_list->list){
					pte_entry = list_entry(pos, struct snapshot_pte_list, list);
					print_pte_debug_info(pte_entry->pte);
					//lets get that page struct that is pointed to by this pte...
					page = pte_page(*(pte_entry->pte));
					//get the pre-existing pte value and clear the pte pointer
					tmp_master_pte = ptep_get_and_clear(vma->vm_mm, pte_entry->addr, pte_entry->pte);
					//we need to write protect the owner's pte again
					tmp_master_pte = pte_wrprotect(tmp_master_pte);	
					//set it back
					set_pte(pte_entry->pte, tmp_master_pte);
					//TODO: TLB flush here?!
					//now lets deal with the page cache, we need to add this page to the page cache if its not in there.
					//first we get the address_space
					mapping = vma->vm_file->f_mapping;
					//spin_lock_irq(&(mapping->tree_lock));
                	//setting the page dirty will ensure that this gets written back to disk
                	SetPageDirty(page);
					//lets see what's in the page cache for this page's index...
					page_test = radix_tree_lookup(&mapping->page_tree, page->index);
					trace_printk("%p %d %p\n", page, (((int )page_test) >> 24), page_test);
					if (page_test != page){	//TODO: Compiler bug here? Why do I have to do this?
		                if (page_test){
		                	//go ahead and delete from the page cache
		                	radix_tree_delete(&mapping->page_tree, page_test->index);
		                	//trace_printk("HUH? %p mapping, %p page tree, %d\n", mapping, mapping->page_tree, page_test);
		                	trace_printk("DID I REALLY GET IN HERE? %d\n", page_test->index );
		                	//this page is no longer associated with the mapping, not SURE if this is needed
							//page_test->mapping = NULL;
		                }
						//spin_unlock_irq(&(mapping->tree_lock));
		               	lock_page(page);
		               	//add it the new page to the page cache
		               	trace_printk("add it the new page to the page cache, %p, %p, %d, %p %p \n", 
		               					page, mapping, page->index, page_test,&mapping->page_tree);
		               	//this may be locked, so we need to unlock it before we go in here
		               	//if (spin_is_locked(&(mapping->tree_lock))){
		               	//	spin_unlock_irq(&(mapping->tree_lock));		//TODO: need a long term fix for this
		               	//}
		               	//add_to_page_cache_locked(page, mapping,page->index, GFP_KERNEL);
		               	//trace_printk("PAGE TREE %p\n", mapping);
		                radix_tree_insert(&mapping->page_tree, page->index, page);
		                page_cache_get(page);
						page->mapping = mapping;
		               	//set the radix tree tag that indicates that the page needs to be written back to disk
		               	radix_tree_tag_set(	&mapping->page_tree,page->index, PAGECACHE_TAG_DIRTY);
		               	//trace_printk("SETTING THE PAGECACHE_TAG_DIRTY? \n");
		               	unlock_page(page);
					}
				}
			}
			/*we need to add a new version list now*/
			new_version_entry = kmalloc(sizeof(struct snapshot_version_list), GFP_KERNEL);
			INIT_LIST_HEAD(&new_version_entry->list);
			new_version_entry->pte_list = NULL;
			new_version_entry->ref_count = 0;
			trace_printk("latest entry is %p\n", new_version_entry);
			/*add the entry to the list*/
			list_add(&new_version_entry->list, &version_list->list);
		}
	}
}

/*This function's purpose is to be an entry point into our snapshot code from the
msync system call. If it's a subscriber, then we grab a snapshot. If it's an owner,
then we need to commit changes*/
void snapshot_msync(struct vm_area_struct * vma){
	if (is_snapshot_read_only(vma)){
		//the purpose of this call was to grab a new snapshot	
		get_snapshot(vma);
	}
	else{
		//the purpose of this call was to commit something
		snapshot_commit(vma);	
	}
}

struct snapshot_version_list * _snapshot_create_version_list(){
	struct snapshot_version_list * version_list = kmalloc(sizeof(struct snapshot_version_list), GFP_KERNEL);
	INIT_LIST_HEAD(&version_list->list);
	/*now we need to add our first entry*/
	struct snapshot_version_list * version_entry = kmalloc(sizeof(struct snapshot_version_list), GFP_KERNEL);
	INIT_LIST_HEAD(&version_entry->list);
	version_entry->pte_list = NULL;
	version_entry->ref_count = 0;
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
	/*get the latest version list, AKA the head of the list*/
	struct snapshot_version_list * latest_version_entry = 
		list_entry( version_list->list.next, struct snapshot_version_list, list);
	
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

void myDebugFunction (struct vm_area_struct * vma, struct mm_struct * mm, struct file * f,  const unsigned long address){

	struct address_space * mapping;
	struct radix_tree_root *root;
	struct page * page_test;
	
	if (f && f->f_mapping){
		mapping = f->f_mapping;
		root = &mapping->page_tree;
		trace_printk("tree root height...%d\n", root->height);
		page_test = radix_tree_lookup(&mapping->page_tree, 0);
		trace_printk("%p\n", page_test);
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
	mmap_snapshot_instance.get_snapshot = snapshot_msync;			//TODO: this function name in the struct should change
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




