#ifndef _JOOL_COMM_NAT64_H
#define _JOOL_COMM_NAT64_H

/**
 * @file
 * Extremely general global stuff.
 *
 * @author Alberto Leiva
 */


#define MODULE_NAME "Jool"

#define STATEFUL 1

/** TODO (fine) #include bool? */
static inline int nat64_is_stateful(void)
{
	return 1;
}

#endif /* _JOOL_COMM_NAT64_H */
