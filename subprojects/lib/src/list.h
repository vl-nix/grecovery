/*

    File: list.h

    Copyright (C) 2006-2008 Christophe GRENIER <grenier@cgsecurity.org>
  
    This software is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
  
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
  
    You should have received a copy of the GNU General Public License along
    with this program; if not, write the Free Software Foundation, Inc., 51
    Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 */
#ifndef _LIST_H
#define _LIST_H

/*
 * These are non-NULL pointers that will result in page faults
 * under normal circumstances, used to verify that nobody uses
 * non-initialized list entries.
 */
#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)

/*
 * Simple doubly linked list implementation.
 * Copied from Linux Kernel 2.6.12.3
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

struct td_list_head {
	struct td_list_head *next, *prev;
};

#ifdef __FRAMAC__
/*@
  @ requires \valid_read(a);
  @ requires \valid_read(b);
  @ assigns \nothing;
  @*/
int file_check_cmp(const struct td_list_head *a, const struct td_list_head *b);

/*@
  @ assigns \nothing;
  @*/
int signature_cmp(const struct td_list_head *a, const struct td_list_head *b);
#endif

/*@
  inductive reachable{L}(struct td_list_head *root, struct td_list_head *node) {
    case root_reachable{L}:
      \forall struct td_list_head *root; reachable(root,root) ;
    case next_reachable{L}:
      \forall struct td_list_head *root,*node;
      \valid(root) && reachable(root->next,node) ==> reachable(root,node);
  }
  @*/
  // root->next->prev == root

/*@ predicate finite{L}(struct td_list_head *root) = reachable(root->next,root); */


/*
      \forall struct td_list_head *l1;
        reachable(l, l1) && \valid(l1) ==> \valid(l1->next) && l1->next->prev == l1;
*/

#define TD_LIST_HEAD_INIT(name) { &(name), &(name) }

#define TD_LIST_HEAD(name) \
	struct td_list_head name = TD_LIST_HEAD_INIT(name)

#define TD_INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
/*
  X requires finite(prev);
  X requires finite(next);
  X ensures finite(prev);
  X ensures finite(next);
  X ensures finite(newe);
  */
/*@
  @ requires \valid(newe);
  @ requires \valid(prev);
  @ requires \valid(next);
  @ requires separation: \separated(newe, \union(prev,next));
  @ requires prev == next || \separated(prev,next,newe);
  @ ensures next->prev == newe;
  @ ensures newe->next == next;
  @ ensures newe->prev == prev;
  @ ensures prev->next == newe;
  @ assigns next->prev,newe->next,newe->prev,prev->next;
  @*/
static inline void __td_list_add(struct td_list_head *newe,
			      struct td_list_head *prev,
			      struct td_list_head *next)
{
	newe->next = next;
	newe->prev = prev;
	next->prev = newe;
	prev->next = newe;
	/*@ assert next->prev == newe; */
	/*@ assert newe->next == next; */
	/*@ assert newe->prev == prev; */
	/*@ assert prev->next == newe; */
	/*X assert finite(prev); */
	/*X assert finite(next); */
	/*X assert finite(newe); */
}

/**
 * td_list_add - add a new entry
 * @newe: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
/*
  X requires finite(head);
  X ensures finite(head);
  X ensures finite(newe);
  */
/*@
  @ requires \valid(newe);
  @ requires \valid(head);
  @ requires \valid(head->next);
  @ requires separation: \separated(newe, \union(head,head->next));
  @ ensures head->next == newe;
  @ ensures newe->prev == head;
  @ ensures newe->next == \old(head->next);
  @ ensures \old(head->next)->prev == newe;
  @ assigns head->next,newe->prev,newe->next,\old(head->next)->prev;
  @*/
static inline void td_list_add(struct td_list_head *newe, struct td_list_head *head)
{
	__td_list_add(newe, head, head->next);
}

#if 0
/*@ type invariant lists(struct td_list_head *l) = finite(l); */
#endif

/**
 * td_list_add_tail - add a new entry
 * @newe: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
/*
  X requires finite(head);
  X ensures finite(head);
  X ensures finite(newe);
  */
/*@
  @ requires \valid(newe);
  @ requires \valid(head);
  @ requires \valid(head->prev);
  @ requires separation: \separated(newe, head);
  @ requires \separated(newe, \union(head->prev, head));
  @ requires head->prev == head || \separated(head->prev, head, newe);
  @ ensures head->prev == newe;
  @ ensures newe->next == head;
  @ ensures newe->prev == \old(head->prev);
  @ ensures \old(head->prev)->next == newe;
  @ assigns head->prev,newe->next,newe->prev,\old(head->prev)->next;
  @*/
static inline void td_list_add_tail(struct td_list_head *newe, struct td_list_head *head)
{
	__td_list_add(newe, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
/*@
  @ requires \valid(prev);
  @ requires \valid(next);
  @ requires prev == next || \separated(prev,next);
  @ ensures next->prev == prev;
  @ ensures prev->next == next;
  @ assigns next->prev,prev->next;
  @*/
static inline void __td_list_del(struct td_list_head * prev, struct td_list_head * next)
{
	next->prev = prev;
	prev->next = next;
	/*@ assert next->prev == prev; */
	/*@ assert prev->next == next; */
}

/**
 * td_list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: td_list_empty on entry does not return true after this, the entry is
 * in an undefined state.
 */
/*@
  @ requires \valid(entry);
  @ requires \valid(entry->prev);
  @ requires \valid(entry->next);
  @ requires \separated(entry, \union(entry->prev,entry->next));
  @ requires entry->prev == entry->next || \separated(entry->prev,entry->next);
  @ ensures  \old(entry->prev)->next == \old(entry->next);
  @ ensures  \old(entry->next)->prev == \old(entry->prev);
  @ assigns \old(entry->prev)->next, \old(entry->next)->prev, entry->next, entry->prev;
  @*/
static inline void td_list_del(struct td_list_head *entry)
{
	__td_list_del(entry->prev, entry->next);
	/*@ assert entry->prev->next == entry->next; */
	/*@ assert entry->next->prev == entry->prev; */
	entry->next = (struct td_list_head*)LIST_POISON1;
	entry->prev = (struct td_list_head*)LIST_POISON2;
	/*@ assert \at(entry->prev,Pre)->next == \at(entry->next,Pre); */
	/*@ assert \at(entry->next,Pre)->prev == \at(entry->prev,Pre); */
}

/**
 * td_list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void td_list_del_init(struct td_list_head *entry)
{
	__td_list_del(entry->prev, entry->next);
	TD_INIT_LIST_HEAD(entry);
}

/**
 * td_list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void td_list_move(struct td_list_head *list, struct td_list_head *head)
{
        __td_list_del(list->prev, list->next);
        td_list_add(list, head);
}

/**
 * td_list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void td_list_move_tail(struct td_list_head *list,
				  struct td_list_head *head)
{
        __td_list_del(list->prev, list->next);
        td_list_add_tail(list, head);
}

/**
 * td_list_empty - tests whether a list is empty
 * @head: the list to test.
 */
/*@
  @ requires \valid_read(head);
  @ assigns  \nothing;
  @*/
static inline int td_list_empty(const struct td_list_head *head)
{
	return head->next == head;
}

/**
 * td_list_empty_careful - tests whether a list is
 * empty _and_ checks that no other CPU might be
 * in the process of still modifying either member
 *
 * NOTE: using td_list_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the list entry is td_list_del_init(). Eg. it cannot be used
 * if another CPU could re-td_list_add() it.
 *
 * @head: the list to test.
 */
static inline int td_list_empty_careful(const struct td_list_head *head)
{
	struct td_list_head *next = head->next;
	return (next == head) && (next == head->prev);
}

static inline void __td_list_splice(struct td_list_head *list,
				 struct td_list_head *head)
{
	struct td_list_head *first = list->next;
	struct td_list_head *last = list->prev;
	struct td_list_head *at = head->next;

	first->prev = head;
	head->next = first;

	last->next = at;
	at->prev = last;
}

/**
 * td_list_splice - join two lists
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void td_list_splice(struct td_list_head *list, struct td_list_head *head)
{
	if (!td_list_empty(list))
		__td_list_splice(list, head);
}

/**
 * td_list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static inline void td_list_splice_init(struct td_list_head *list,
				    struct td_list_head *head)
{
	if (!td_list_empty(list)) {
		__td_list_splice(list, head);
		TD_INIT_LIST_HEAD(list);
	}
}

/**
 * td_list_entry - get the struct for this entry
 * @ptr:	the &struct td_list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the td_list_struct within the struct.
 */
#define td_list_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(size_t)(&((type *)0)->member)))

#define td_list_entry_const(ptr, type, member) \
	((type *)((const char *)(ptr)-(size_t)(&((type *)0)->member)))

/**
 * __td_list_for_each	-	iterate over a list
 * @pos:	the &struct td_list_head to use as a loop counter.
 * @head:	the head for your list.
 *
 */
#define td_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * td_list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct td_list_head to use as a loop counter.
 * @head:	the head for your list.
 */
#define td_list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); \
        	pos = pos->prev)

/**
 * td_list_for_each_safe	-	iterate over a list safe against removal of list entry
 * @pos:	the &struct td_list_head to use as a loop counter.
 * @n:		another &struct td_list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define td_list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * td_list_for_each_prev_safe	-	iterate over a list backwards safe against removal of list entry
 * @pos:	the &struct td_list_head to use as a loop counter.
 * @n:		another &struct td_list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define td_list_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; pos != (head); \
		pos = n, n = pos->prev)

/**
 * td_list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the td_list_struct within the struct.
 */
#define td_list_for_each_entry(pos, head, member)				\
	for (pos = td_list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = td_list_entry(pos->member.next, typeof(*pos), member))

/**
 * td_list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the td_list_struct within the struct.
 */
#define td_list_for_each_entry_reverse(pos, head, member)			\
	for (pos = td_list_entry((head)->prev, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = td_list_entry(pos->member.prev, typeof(*pos), member))

/**
 * td_list_prepare_entry - prepare a pos entry for use as a start point in
 *			td_list_for_each_entry_continue
 * @pos:	the type * to use as a start point
 * @head:	the head of the list
 * @member:	the name of the td_list_struct within the struct.
 */
#define td_list_prepare_entry(pos, head, member) \
	((pos) ? : td_list_entry(head, typeof(*pos), member))

/**
 * td_list_for_each_entry_continue -	iterate over list of given type
 *			continuing after existing point
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the td_list_struct within the struct.
 */
#define td_list_for_each_entry_continue(pos, head, member) 		\
	for (pos = td_list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head);	\
	     pos = td_list_entry(pos->member.next, typeof(*pos), member))

/**
 * td_list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop counter.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the td_list_struct within the struct.
 */
#define td_list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = td_list_entry((head)->next, typeof(*pos), member),	\
		n = td_list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = td_list_entry(n->member.next, typeof(*n), member))


/**
 * td_list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */

#define td_list_first_entry(ptr, type, member) \
	td_list_entry((ptr)->next, type, member)


/**
 * td_list_last_entry - get the last element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_head within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define td_list_last_entry(ptr, type, member) \
	td_list_entry((ptr)->prev, type, member)


/**
 * td_list_next_entry - get the next element in list
 * @pos:	the type * to cursor
 * @member:	the name of the list_head within the struct.
 */
#define td_list_next_entry(pos, member) \
	td_list_entry((pos)->member.next, typeof(*(pos)), member)

/**
 * td_list_prev_entry - get the prev element in list
 * @pos:	the type * to cursor
 * @member:	the name of the list_head within the struct.
 */
#define td_list_prev_entry(pos, member) \
	td_list_entry((pos)->member.prev, typeof(*(pos)), member)


#if 1
/*
  X requires finite(head);
  X ensures  finite(head);
  X ensures  finite(newe);
  X ensures  reachable(head,newe);
  */
/*@
  @ requires \valid(newe);
  @ requires \valid(head);
  @ requires \valid_function(compar);
  @ requires compar == signature_cmp || compar == file_check_cmp;
  @ requires separation: \separated(newe, head);
  @*/
static inline void td_list_add_sorted(struct td_list_head *newe, struct td_list_head *head,
    int (*compar)(const struct td_list_head *a, const struct td_list_head *b))
{
  struct td_list_head *pos;
  /*@
    @ loop invariant pos == head || \separated(pos, head);
    @*/
  td_list_for_each(pos, head)
  {
    /*@ assert compar == signature_cmp || compar == file_check_cmp; */
    /*@ assert \valid_read(newe); */
    /*@ assert \valid_read(pos); */
    /*X calls signature_cmp, file_check_cmp; */
    /*@ calls file_check_cmp; */
    if(compar(newe,pos)<0)
    {
      __td_list_add(newe, pos->prev, pos);
      /*X assert finite(head); */
      /*X assert finite(newe); */
      return ;
    }
  }
  td_list_add_tail(newe, head);
  /*X assert finite(head); */
  /*X assert finite(newe); */
}
#endif

/*@
  @ requires \valid(newe);
  @ requires \valid(head);
  @ requires \valid_function(compar);
  @ requires separation: \separated(newe, head);
  @*/
static inline int td_list_add_sorted_uniq(struct td_list_head *newe, struct td_list_head *head,
    int (*compar)(const struct td_list_head *a, const struct td_list_head *b))
{
  struct td_list_head *pos;
  /*@
    @ loop invariant pos == head || \separated(pos, head);
    @ loop assigns pos;
    @*/
  td_list_for_each(pos, head)
  {
    // TODO const
    /* calls spacerange_cmp; */
    int res=compar(newe,pos);
    if(res<0)
    {
      __td_list_add(newe, pos->prev, pos);
      return 0;
    }
    else if(res==0)
      return 1;
  }
  td_list_add_tail(newe, head);
  return 0;
}

typedef struct alloc_list_s alloc_list_t;
struct alloc_list_s
{
  struct td_list_head list;
  uint64_t start;
  uint64_t end;
  unsigned int data;
};

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif
#endif
