

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

#define SNAPSHOT_PREFIX "snapshot"
#define SNAPSHOT_DEBUG Y

/*Policies specified by the subscribers*/
#define COMMIT_ALWAYS	0x100000
#define COMMIT_ADAPT	0x200000
#define COMMIT_DYN		0x400000
#define COMMIT_INTERVAL_SHIFT 23	

/*flag for snapshots*/
#define SNAP_BLOCKING		0x10
#define SNAP_NONBLOCKING	0x20

/*Used for doing the exponentially weighted moving average*/
#define ADAPT_EWMA_COEFF		20
#define ADAPT_EWMA_FACTOR		100		
#define ADAPT_EWMA_DEFAULT_US 	1000
/*How old is too old for a snapshot when using adapt?*/
#define ADAPT_AGE_FACTOR		3

struct snapshot_version_list * _snapshot_create_version_list();

/*TODO: Put this in the header file*/

//struct rw_semaphore rwsem_snapversionlist;
static DEFINE_SEMAPHORE(rwsem_snapversionlist);

//for debugging only
int snapshot_pf_count=0;
int page_counter=0;
unsigned long total_commit_time=0;
unsigned long total_commits=0;

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

/*this structure keeps track of commit priorities, when should an owner commit?*/
struct commit_prio_list{
	struct list_head list;
	unsigned long microseconds;
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
	
	if (vma && vma->vm_file && vma->vm_file->f_path.dentry){
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

unsigned long elapsed_time_in_microseconds(struct timeval * old_tv, struct timeval * current_tv){

	unsigned long secs;
	//get current time
	do_gettimeofday(current_tv);
	secs = current_tv->tv_sec - old_tv->tv_sec;
	trace_printk("current secs %lu, usecs %lu, old secs %lu, usecs %lu\n", current_tv->tv_sec, current_tv->tv_usec, old_tv->tv_sec, old_tv->tv_usec);
	if (secs){
		return ((secs * 1000000) - old_tv->tv_usec) + current_tv->tv_usec;
	}
	else
		return current_tv->tv_usec - old_tv->tv_usec;
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
	struct page * page, * old_page;	
	unsigned long readonly_addr;
	int page_mapped_result = 0;
	pte_t * dest_pte;
	pte_t tmp_ro_pte, tmp_master_pte;
	
	page = pte_page(*pte);
	if (page){
		//is this page mapped into our address space?
		page_mapped_result = page_mapped_in_vma(page, vma_read_only);	//TODO: is this necessary?
		if (!page_mapped_result){
			lock_page(page);
			readonly_addr = (page->index << PAGE_SHIFT) + vma_read_only->vm_start;	//get the new address
			dest_pte = get_pte_entry_from_address( vma_read_only->vm_mm, readonly_addr);
			old_page = pte_page(*dest_pte);	//getting the page struct for the pte we just grabbed
			if (dest_pte && old_page){
				//do this only if the old page is actually mapped into our address space
				if (page_mapped_in_vma(old_page, vma_read_only)){		
					lock_page(old_page);
					//remove from the rmap
					page_remove_rmap(old_page);
					old_page->mapping=NULL;			//TODO: perhaps paranoia?
					/*trace_printk("MAPCOUNT in copy: address %08lx, PTE VAL %lu, and count is %d and ref count is %d\n", 
							addr, pte_val(*dest_pte), page_mapcount(old_page), page_count(old_page));*/
					unlock_page(old_page);
					//decrement the ref count of the old page
					//printk(KERN_ALERT " before put_page in copy pte\n");
					//dump_page(old_page);
					if (atomic_read(&old_page->_count) == 2){
						page_counter--;
						//trace_printk("PAGE-COUNTER: %d GET-SUB %p %d\n", page_counter, old_page, atomic_read(&old_page->_count));
					}
					put_page(old_page);	
				}
				else{
					//printk(KERN_ALERT "in copy...old page wasn't mapped in\n");	
				}
				
				//increment the ref count for this page
				get_page(page);	
				//add the page to the rmap
				if (PageAnon(page)){
					//printk("the page is anonymous\n");
					page_add_anon_rmap(page, vma_read_only, addr);
				}							
				else{
					page_add_file_rmap(page);
				}
				//trace_printk("MAPCOUNT in copy: address %08lx, PTE VAL %lu, and count is %d and ref count is %d\n", 
				//addr, pte_val(*pte), page_mapcount(page), page_count(page));
				//create the new pte given a physical page (frame)
				tmp_ro_pte = mk_pte(page, vma_read_only->vm_page_prot);
				//clear the accessed bit
				pte_mkold(tmp_ro_pte);
				//flush the tlb for this page
				__flush_tlb_one(addr);
				//flush_tlb_page(vma_read_only, addr); 
				
				//now set the new pte
				set_pte(dest_pte, tmp_ro_pte);
				//tmp_master_pte = ptep_get_and_clear(mm, addr, pte);
				//we need to write protect the owner's pte again
				//tmp_master_pte = pte_wrprotect(tmp_master_pte);	
				//set the mapping back so it's no longer anonymous
				page->mapping = vma_read_only->vm_file->f_path.dentry->d_inode->i_mapping;	//TODO: replace this with the "better" path
				//set_pte(pte, tmp_master_pte);
				pte_unmap(dest_pte);	
				//page_counter++;
				//trace_printk("PAGE-COUNTER: %d GET-ADD: %p : %d\n", page_counter, page, atomic_read(&page->_count));
				
			}
			unlock_page(page);						
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
	return (target && ls && staged_version_list(target->prev, ls) );
}


/*should we actually do the snapshot? This function is only important if this is a blocking request*/
int do_snapshot (struct vm_area_struct * master_vma, struct vm_area_struct * subscriber_vma){
	
	unsigned long elapsed_microseconds;
	struct timeval tmp_tv;
	
	if (atomic_read(&subscriber_vma->always_ref_count)){
		trace_printk("do_commit: with ALWAYS or nonblocking\n");
		/*OK, we want to get a snapshot no matter what*/
		return 1;	
	}	
	
	/*if it's the first snapshot, go ahead and do it*/
	if (!subscriber_vma->snapshot_pte_list){
		trace_printk("its the first one\n");
		return 1;
	}
		
	/*if it's COMMIT_ADAPT or COMMIT_DYN we need to look at the time*/
	elapsed_microseconds = elapsed_time_in_microseconds(&master_vma->last_commit_time, &tmp_tv);
	/*Is it dynamic????*/
	if (subscriber_vma->ewma_adapt){
		trace_printk("it's dynamic, %lu, elapsed time %lu\n", subscriber_vma->ewma_adapt, elapsed_microseconds);
		/*is the last commit too old?*/
		if (elapsed_microseconds >= subscriber_vma->ewma_adapt){
			return 0;	
		}
	}	
	/*Is it adapt*/
	else if (atomic_read(&subscriber_vma->adapt_ref_count)){
		trace_printk("it's adapt, %lu, elapsed time %lu\n", master_vma->ewma_adapt, elapsed_microseconds);
		if (elapsed_microseconds >= master_vma->ewma_adapt * ADAPT_AGE_FACTOR){
			return 0;	
		}
	}
	
	return 1;
}

void wait_for_commit(struct vm_area_struct * master_vma, struct vm_area_struct * subscriber_vma){
	int revision_number = atomic_read(&master_vma->revision_number);
	trace_printk("In wait_for_commit\n");
	if (!do_snapshot(master_vma, subscriber_vma)){
		/*OK, we are blocking and waiting until we get a new commit*/
		DECLARE_WAITQUEUE(wait, current);
		add_wait_queue(&master_vma->snapshot_wq, &wait);
		/*Keep going around in this loop until someone (the owner) commits*/
		trace_printk("the revision # is %d\n", revision_number);
		while(atomic_read(&master_vma->revision_number) == revision_number){
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
			trace_printk("got woken up, %d, %d\n", revision_number, atomic_read(&master_vma->revision_number));
		}
		set_current_state(TASK_RUNNING);
		remove_wait_queue(&master_vma->snapshot_wq, &wait);
	}
}

void update_master_adapt_ewma (struct vm_area_struct * master_vma){

	unsigned long elapsed_time;
	struct timeval tmp_tv;
	
	if (master_vma->last_snapshot_time.tv_sec == 0){
		do_gettimeofday(&master_vma->last_snapshot_time);		
		return;
	}
	
	elapsed_time = elapsed_time_in_microseconds(&master_vma->last_snapshot_time, &tmp_tv);
	trace_printk("IN update_master_adapt_ewma: elapsed time is %lu, ewma_adapt is %lu\n", elapsed_time, master_vma->ewma_adapt);
	memcpy(&master_vma->last_snapshot_time, &tmp_tv, sizeof(struct timeval));
	/*update the ewma*/
	master_vma->ewma_adapt =  	(	(ADAPT_EWMA_COEFF * elapsed_time) + 
									((ADAPT_EWMA_FACTOR - ADAPT_EWMA_COEFF) * master_vma->ewma_adapt)
								)/ADAPT_EWMA_FACTOR;
	
	trace_printk("new ewma_adapt is %lu\n", master_vma->ewma_adapt);
}

int get_snapshot (struct vm_area_struct * vma, unsigned long flags){
	struct vm_area_struct * master_vma;
	/*for iterating through the list*/
	struct list_head * pos, * pos_outer, * pos_outer_tmp, * ls, * new_list;
	/*for storing pte values from list*/
	struct snapshot_pte_list * tmp_pte_list;
	struct snapshot_version_list * latest_version_entry;
	
	
	//trace_printk("SNAP: get_snapshot, blocking? %lu, non-blocking %lu, %lu \n", flags & SNAP_BLOCKING, flags & SNAP_NONBLOCKING, flags);
	trace_printk("PAGE-COUNTER: %d\n", page_counter);
	
	if (vma && vma->vm_mm && vma->vm_file){
		master_vma = get_master_vma(vma->vm_file, vma);
		if (master_vma){
			/*update the adapt EWMA*/
			update_master_adapt_ewma(master_vma);
			/*should we actually grab the snapshot?*/
			if (flags & SNAP_BLOCKING){
				wait_for_commit(master_vma, vma);
			}
			//trace_printk("OK, we are going to do this thing!\n");
			//grab the lock
			down(&rwsem_snapversionlist);
			if (master_vma->snapshot_pte_list){
				//get the latest version list
				struct snapshot_version_list * version_list = (struct snapshot_version_list *)master_vma->snapshot_pte_list;
				//only go in here if there is more than one element in the list (prev != next)
				if (version_list && version_list->list.prev != version_list->list.next){
					//if the subscriber's vma has a previous ptr into the list, use that. If not, just use the entire list
					if (vma->snapshot_pte_list){
						ls = (struct list_head *)vma->snapshot_pte_list;
						//trace_printk("using the old list!\n");
						//if we are already the latest committed version, no reason to go through all of this
						if (last_committed_version_list(ls,&version_list->list)){
							//trace_printk("we are the latest committed version!\n");
							up(&rwsem_snapversionlist);
							trace_printk("PAGE-COUNTER: %d\n", page_counter);
							return 1;
						}
					}
					else{
						//no previous ptr to list, just use the whole thing
						//trace_printk("using the whole thing!\n");
						ls = &version_list->list;
					}
					list_for_each_prev_safe(pos_outer, pos_outer_tmp, ls){
						if (staged_version_list(pos_outer,&version_list->list)){
							//trace_printk("got the staged version\n");
							break;
						}
						
						latest_version_entry = list_entry( pos_outer, struct snapshot_version_list, list);
						//trace_printk("got latest version entry 2 %p %p\n", latest_version_entry, latest_version_entry->pte_list);
						if (latest_version_entry && latest_version_entry->pte_list){
							//trace_printk("HHUH???\n");
							//now traverse the pte list to see what has changed.
							if ( list_empty(&latest_version_entry->pte_list->list)){
								//trace_printk("deleted a list\n");
								//list_del(pos_outer);
							}
							else{
								//trace_printk("HERE???\n");
								list_for_each(pos, &latest_version_entry->pte_list->list){
									//get the pte entry
									//trace_printk("HERE 2???\n");
									tmp_pte_list = list_entry(pos, struct snapshot_pte_list, list);
									if (tmp_pte_list){
										//trace_printk("HERE 3???\n");
										//now actually perform the copy
										//trace_printk("actually calling copy_pte_entry\n");
										copy_pte_entry (tmp_pte_list->pte, tmp_pte_list->addr, vma, master_vma->vm_mm);
									}
								}
								new_list = pos_outer;
							}
						}
					}
					vma->snapshot_pte_list = new_list;
				}
			}
			//release the semaphore
			up(&rwsem_snapversionlist);
		}
	}
	
	trace_printk("PAGE-COUNTER: %d\n", page_counter);
	
	return 0;
}

void delete_old_pte(struct radix_tree_root * snapshot_page_tree, unsigned long index){
	
	struct snapshot_pte_list * pte_entry; 
	struct list_head * pte_entry_ls;
	
	//try and delete from the pte radix tree
	if ((pte_entry_ls = radix_tree_delete(snapshot_page_tree, index))){
		//for debugging, who was this. TODO: get rid of this
		pte_entry = list_entry(pte_entry_ls, struct snapshot_pte_list, list);
		trace_printk("delete_pte_from_list: the address was...%08lx\n", pte_entry->addr);
		//there was something in there...now delete that list entry
		list_del(pte_entry_ls);
		kfree(pte_entry);
	}
}

/*Should we perform the commit or not depending on the behavior specified by the subscribers*/
int do_commit(struct vm_area_struct * vma){
	struct list_head * pos;
	struct commit_prio_list * commit_interval;
	unsigned long elapsed_microseconds;
	struct timeval tmp_tv;
	
	trace_printk("in do_commit\n");
	/*if nothing is setup, then commit (TODO: This is only for testing...should be removed)*/
	if (atomic_read(&vma->always_ref_count) == 0 && atomic_read(&vma->adapt_ref_count) == 0 
		&& list_empty(&vma->prio_list)){
		trace_printk("do_commit: nothing setup yet by subscribers, just commit\n");
		return 1;		
	}
	
	if (atomic_read(&vma->always_ref_count)){
		trace_printk("do_commit: with ALWAYS\n");
		/*OK, we always want to commit no matter what*/
		return 1;	
	}	
	/*if it's the first one, go ahead and do it*/
	else if (vma->last_commit_time.tv_sec == 0){
		trace_printk("its the first one\n");
		return 1;
	}
	else{
		
		elapsed_microseconds = elapsed_time_in_microseconds(&vma->last_commit_time, &tmp_tv);
		
		trace_printk("elapsed microseconds...%lu\n", elapsed_microseconds);
		if (atomic_read(&vma->adapt_ref_count)){
			trace_printk("do_commit: with ADAPT\n");
			/*OK, we need to use the adaptive approach (how often are we getting requests from subscribers?*/
			return 1;
		}
		else{
			trace_printk("do_commit: trying dynamic\n");
			list_for_each(pos, &vma->prio_list){
				commit_interval = list_entry(pos, struct commit_prio_list, list);
				trace_printk("the interval is %lu and elapsed time is %lu, %p %p\n", 
				commit_interval->microseconds, elapsed_microseconds, commit_interval, &vma->prio_list);
				if (commit_interval && elapsed_microseconds > commit_interval->microseconds){
					trace_printk("the interval is %lu and elapsed time is %lu\n", commit_interval->microseconds, elapsed_microseconds);
					return 1;
				}
				else{
					trace_printk("the interval is %lu and elapsed time is %lu\n", commit_interval->microseconds, elapsed_microseconds);
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
	struct snapshot_version_list * latest_version_entry, * new_version_entry, * version_list;
	struct snapshot_pte_list * pte_entry;
	pte_t tmp_master_pte;
	struct page * page;
	struct page * page_test;
	struct address_space * mapping;
	int insert_error;
	int commit_pte_counter;
	
	struct timeval start_commit_tv, end_commit_tv;
	
	/*for stats*/
	do_gettimeofday(&start_commit_tv);
	commit_pte_counter=0;
	
	//trace_printk("PAGE-COUNTER: %d\n", page_counter);
	
	if (vma && vma->snapshot_pte_list && do_commit(vma)){
		//trace_printk("IN COMMIT \n");
		version_list = (struct snapshot_version_list *)vma->snapshot_pte_list;
		if (version_list && version_list->list.prev){
			//get the latest list of modifications that haven't been committed (at the head of the list)
			latest_version_entry = list_entry(version_list->list.next, struct snapshot_version_list, list);
			//now, traverse the list
			//trace_printk("IN COMMIT, before the list \n");
			down(&rwsem_snapversionlist);
			if (latest_version_entry && latest_version_entry->pte_list){
				//trace_printk("IN COMMIT, do we have a list? \n");
				list_for_each(pos, &latest_version_entry->pte_list->list){
					commit_pte_counter++;
					pte_entry = list_entry(pos, struct snapshot_pte_list, list);
					//lets get that page struct that is pointed to by this pte...
					page = pte_page(*(pte_entry->pte));
					//get the pre-existing pte value and clear the pte pointer
					tmp_master_pte = ptep_get_and_clear(vma->vm_mm, pte_entry->addr, pte_entry->pte);
					//flush the cache
					flush_tlb_page(vma, pte_entry->addr);
					//we need to write protect the owner's pte again
					tmp_master_pte = pte_wrprotect(tmp_master_pte);	
					//set it back
					set_pte(pte_entry->pte, tmp_master_pte);
					//now lets deal with the page cache, we need to add this page to the page cache if its not in there.
					//first we get the address_space
					mapping = vma->vm_file->f_mapping;
					//spin_lock_irq(&(mapping->tree_lock));
                	//setting the page dirty will ensure that this gets written back to disk
                	SetPageDirty(page);
					//lets see what's in the page cache for this page's index...
					page_test = radix_tree_lookup(&mapping->page_tree, page->index);
					if (page_test != page){
		                if (page_test){
		                	lock_page(page_test);
		                	//go ahead and delete from the page cache
		                	radix_tree_delete(&mapping->page_tree, page_test->index);
		                	//if it's in the page cache, then it's probably already in the pte cache, try and delete
		                	delete_old_pte(&vma->snapshot_page_tree, page->index); 
		                	page_test->mapping=NULL;	                	
		                	unlock_page(page_test);
		                	//trace_printk("DELETE REF COUNT IS: %d\n", atomic_read(&page_test->_count));
		                	if (atomic_read(&page_test->_count) == 1){
								page_counter--;
								//trace_printk("PAGE-COUNTER: %d MAKE %p %d\n", page_counter, page_test, atomic_read(&page_test->_count));
							}
							else{
								//trace_printk("PAGE-COUNTER: %d MAKE-NO-SUB %p %d\n", page_counter, page_test, atomic_read(&page_test->_count));
							}
		                	
		                }
retry_insert:
		                //lets now add our new pte into the pte radix tree
		                insert_error = radix_tree_insert(&vma->snapshot_page_tree, page->index, pos);
		                if (insert_error == -EEXIST){
		                	delete_old_pte(&vma->snapshot_page_tree, page->index);
		                	goto retry_insert;
		                }
		                //if it wasn't an eexist error, then something is wrong and we have a bug
		                BUG_ON((insert_error && insert_error != -EEXIST));
		                
		               	lock_page(page);
		               	//add it the new page to the page cache
		                radix_tree_insert(&mapping->page_tree, page->index, page);
		                //page_cache_get(page);
						page->mapping = mapping;
		               	//set the radix tree tag that indicates that the page needs to be written back to disk
		               	radix_tree_tag_set(	&mapping->page_tree,page->index, PAGECACHE_TAG_DIRTY);
		               	ClearPageSwapBacked(page);
		               	/*trace_printk("MAPCOUNT In Commit delete: address %08lx, and count is %d and refcount is %d \n", pte_entry->addr, 
		               			page_mapcount(page), page_count(page));*/
		               	unlock_page(page);
		               	
					}
				}
			}
			
			/*we need to add a new version list now*/
			new_version_entry = kmalloc(sizeof(struct snapshot_version_list), GFP_KERNEL);
			INIT_LIST_HEAD(&new_version_entry->list);
			new_version_entry->pte_list = NULL;
			//trace_printk("latest entry is %p\n", new_version_entry);
			/*add the entry to the list*/
			list_add(&new_version_entry->list, &version_list->list);
			/*set the commit time*/
			do_gettimeofday(&vma->last_commit_time);
			/*up the revision count*/
			atomic_inc(&vma->revision_number);
			up(&rwsem_snapversionlist);
			/*now we need to wake up anyone that is waiting for a new commit*/
			wake_up(&vma->snapshot_wq);
		}
	}
	
	do_gettimeofday(&end_commit_tv);
	total_commit_time+=elapsed_time_in_microseconds(&start_commit_tv, &end_commit_tv);
	total_commits++;
	
	trace_printk("PAGE-COUNTER: %d\n", page_counter);
	
	trace_printk("TOTAL-COMMIT-TIME: %lu AVG-COMMIT-TIME: %lu TOTAL-COMMITS: %lu\n", 
				total_commit_time, total_commit_time/total_commits, total_commits);
	//trace_printk("avg. commit latency: %lu\n", elapsed_time_in_microseconds(&start_commit_tv, &end_commit_tv));
	trace_printk("ptes copied: %d\n", commit_pte_counter);
}

/*This function's purpose is to be an entry point into our snapshot code from the
msync system call. If it's a subscriber, then we grab a snapshot. If it's an owner,
then we need to commit changes*/
void snapshot_msync(struct vm_area_struct * vma, unsigned long flags){
	if (is_snapshot_read_only(vma)){
		//the purpose of this call was to grab a new snapshot	
		get_snapshot(vma, flags);
	}
	else{
		//the purpose of this call was to commit something
		snapshot_commit(vma);	
	}
}

struct snapshot_version_list * _snapshot_create_version_list(){
	struct snapshot_version_list * version_list, * version_entry;
	
	version_list = kmalloc(sizeof(struct snapshot_version_list), GFP_KERNEL);
	INIT_LIST_HEAD(&version_list->list);
	/*now we need to add our first entry*/
	version_entry = kmalloc(sizeof(struct snapshot_version_list), GFP_KERNEL);
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

int init_subscriber (struct vm_area_struct * vma, unsigned long flags){
	
	struct vm_area_struct * master_vma;
	struct commit_prio_list * head;
	
	/*get the master (owner) vma*/
	master_vma = get_master_vma(vma->vm_file, vma);
	if (master_vma){
		/*initialize these to zero*/
		atomic_set(&vma->adapt_ref_count,0);
		atomic_set(&vma->always_ref_count,0);
		vma->ewma_adapt = 0;
		if (flags & COMMIT_DYN){
			trace_printk("DYNAMIC!!!!\n");
			/*we need to add a new commit to the list*/
			head = kmalloc(sizeof(struct commit_prio_list), GFP_KERNEL);
			INIT_LIST_HEAD(&head->list);
			/*get the interval from the flags, which is COMMIT_INTERVAL_SHIFT bits to the left*/
			head->microseconds = vma->ewma_adapt = (flags >> COMMIT_INTERVAL_SHIFT) * 100;	/*TODO: Change ewma_adapt here*/
			list_add(&head->list,&master_vma->prio_list);
			trace_printk("the commit_interval is %lu in microseconds\n", head->microseconds);
		}	
		else if (flags & COMMIT_ADAPT){
			trace_printk("ADAPT!!!!\n");
			atomic_inc(&master_vma->adapt_ref_count);
			atomic_inc(&vma->adapt_ref_count);
		}
		else if (flags & COMMIT_ALWAYS){
			trace_printk("ALWAYS!!!!\n");
			atomic_inc(&master_vma->always_ref_count);
			atomic_inc(&vma->always_ref_count);
		}
	}
	
	return 1;
}



int init_snapshot (struct vm_area_struct * vma){
	struct snapshot_version_list * version_list;
	/*do we need to initialize the list?*/
	/*create a new version list node*/
	version_list = _snapshot_create_version_list();
	/*setting the ptr on the vma to point at our new list*/
	vma->snapshot_pte_list = version_list;
	/*now, lets initialize the radix tree for mapping index to pte*/
	INIT_RADIX_TREE(&vma->snapshot_page_tree, GFP_KERNEL);
	/*setup the wait queue used for blocking snapshot requests*/
	init_waitqueue_head(&vma->snapshot_wq);
	/*initialize the priority list*/
	INIT_LIST_HEAD(&vma->prio_list);
	/*initialize the time since last commit*/
	vma->last_commit_time.tv_sec = vma->last_commit_time.tv_usec = 0;
	/*initialize the last snapshot time*/
	vma->last_snapshot_time.tv_sec = vma->last_snapshot_time.tv_usec = 0;
	/*set the revision number to zero*/
	atomic_set(&vma->revision_number,0);
	atomic_set(&vma->adapt_ref_count,0);
	atomic_set(&vma->always_ref_count,0);
	/*initialize the ewma to the defined default*/
	vma->ewma_adapt = ADAPT_EWMA_DEFAULT_US;
	
	snapshot_pf_count=0;
	page_counter=(vma->vm_end - vma->vm_start)/(1<<12);
	//page_counter=0;
	
	trace_printk("page counter starts at....%d, end %p, start %p\n", page_counter, vma->vm_end, vma->vm_start);
	
	total_commit_time=0;
	total_commits=0;
	
	printk(KERN_ALERT "\n\n\n\n\n\n\n\n\n\nSTARTING THIS PARTY\n");
	
	return 1;
}

int do_snapshot_add_pte (struct vm_area_struct * vma, pte_t * orig_pte, pte_t * new_pte, unsigned long address){
	
	struct prio_tree_iter iter;
	struct vm_area_struct * tmp_vma;
	struct snapshot_version_list * version_list, * latest_version_entry, * tmp_version_list;
	struct snapshot_pte_list * pte_list_entry;
	struct page * page_debug;
	
	//trace_printk("SNAP: in do_snapshot_add_pte\n");
	/*this should never happen */
	if (!vma->snapshot_pte_list){
		//trace_printk("SNAP: creating new snapshot pte list\n");
		/*create a new version list node*/
		tmp_version_list = _snapshot_create_version_list();
		/*setting the ptr on the vma to point at our new list*/
		vma->snapshot_pte_list = tmp_version_list;
	}
	/*get version list from vma*/
	version_list = vma->snapshot_pte_list;
	/*get the latest version list, AKA the head of the list*/
	latest_version_entry = 
		list_entry( version_list->list.next, struct snapshot_version_list, list);
	
	/*if we are the first page fault, we have to create the pte list*/
	//trace_printk("SNAP: version_list->list.prev %p %p\n", version_list->list.prev,  version_list);
	if (!latest_version_entry->pte_list){
		latest_version_entry->pte_list = _snapshot_create_pte_list();
	}
	
	/*create the new pte entry*/
	pte_list_entry = kmalloc(sizeof(struct snapshot_pte_list), GFP_KERNEL);
	INIT_LIST_HEAD(&pte_list_entry->list);
	pte_list_entry->pte = new_pte;
	pte_list_entry->addr = address;
	/*now we need to add the pte to the list */
	list_add(&pte_list_entry->list, &latest_version_entry->pte_list->list);

	/*trace_printk("the current vma is %p, the mapping is %p\n", vma, &vma->vm_file->f_mapping->i_mmap);
	vma_prio_tree_foreach(tmp_vma, &iter, &vma->vm_file->f_mapping->i_mmap, 0, ULONG_MAX){
		trace_printk("got vma: %p\n", tmp_vma);
	}*/
	
	page_debug = pte_page(*new_pte);
	if (page_debug){
		trace_printk("MAPCOUNT In Add PTE: address %08lx, and count is %d and refcount is %d \n", address, 
		               			page_mapcount(page_debug), page_count(page_debug));
	}
	
	snapshot_pf_count+=1;
	page_counter++;
	return 1;
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
	mmap_snapshot_instance.snapshot_msync = snapshot_msync;			//TODO: this function name in the struct should change
	mmap_snapshot_instance.init_snapshot = init_snapshot;
	mmap_snapshot_instance.do_snapshot_add_pte = do_snapshot_add_pte;
	mmap_snapshot_instance.init_subscriber = init_subscriber;
	
	
	return 0;
}

void cleanup_module(void)
{
	tim_debug_instance.tim_get_unmapped_area_debug=NULL;
	tim_debug_instance.tim_unmap_debug=NULL;
	
	mmap_snapshot_instance.is_snapshot = NULL;
	mmap_snapshot_instance.is_snapshot_read_only = NULL;
	mmap_snapshot_instance.is_snapshot_master = NULL;
	mmap_snapshot_instance.snapshot_msync = NULL;
	mmap_snapshot_instance.init_snapshot = NULL;
	mmap_snapshot_instance.do_snapshot_add_pte = NULL;
	mmap_snapshot_instance.init_subscriber = NULL;
	
	printk(KERN_INFO "Goodbye world 1.\n");
	
}




