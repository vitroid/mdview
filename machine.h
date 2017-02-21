/* machine.h */

#ifndef _MACHINE_H
#define _MACHINE_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* size_t */
#if STDC_HEADERS
# include <sys/types.h>
# include <stdlib.h>
#else
# error the ANSI headers are required.
#endif

extern size_t Strlcpy(char *dst, const char *src, size_t size);
extern size_t Strlcat(char *dst, const char *src, size_t size);

extern void GetTimeval(long *sec, long *usec);
extern void Usleep(long utime);

#endif /* _MACHINE_H */
