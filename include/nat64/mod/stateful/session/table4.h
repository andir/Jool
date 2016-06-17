#ifndef _JOOL_MOD_SESSION_TABLE4_H
#define _JOOL_MOD_SESSION_TABLE4_H

/**
 * The IPv4 index of the sessions.
 * It's a tree-ish data structure that sorts them by src4, then by dst4.l3 and
 * then by dst4.l4.
 */

#include "nat64/mod/common/types.h"
#include "nat64/mod/stateful/bib/table.h"
#include "nat64/mod/stateful/session/entry.h"

struct session_table4;

struct session_table4 *st4_create(void);
void st4_destroy(struct session_table4 *table);

int st4_find(struct session_table4 *table, struct tuple *tuple4,
		struct bib_entry *bib, struct session_entry **session,
		bool *allow);
int st4_find_bib(struct session_table4 *table, struct tuple *tuple4,
		struct bib_entry *bib);

int st4_add(struct session_table4 *table, struct session_entry *session);
void st4_rm(struct session_table4 *table, struct session_entry *session);
void st4_flush(struct session_table4 *table);

typedef void (*st4_destructor_cb)(struct session_entry *);
void st4_prune_src4(struct session_table4 *table,
		struct ipv4_transport_addr *src4,
		st4_destructor_cb destructor);

void st4_print(struct session_table4 *table);

#endif /* _JOOL_MOD_SESSION_TABLE4_H */
