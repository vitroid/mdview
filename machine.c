/* "machine.c" 処理系依存部分 */

#include "machine.h"

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#endif

/*----------------------------Strlcpy(),Strlcat()----------------------------*/
#if HAVE_STRLCPY

# include <string.h>
size_t Strlcpy(char *dst, const char *src, size_t size) {
  return strlcpy(dst, src, size);
}
size_t Strlcat(char *dst, const char *src, size_t size) {
  return strlcat(dst, src, size);
}

#else

size_t Strlcpy(char *dst, const char *src, size_t size) {
  register size_t i;

  /* copy */
  for (i = 0; i < size && (dst[i] = src[i]) != '\0'; i++)
    ;
  if (i < size) return i; /* count does not include NUL */

  /* truncate */
  if (size > 0) dst[size-1] = '\0';
  for (; src[i] != '\0'; i++)
    ;
  return i; /* count does not include NUL */
}

size_t Strlcat(char *dst, const char *src, size_t size) {
  register size_t i;

  /* read */
  for (i = 0; i < size && dst[i] != '\0'; i++)
    ;
  if (i >= size) {
    /* too long dst[] */
    for (i = 0; src[i] != '\0'; i++)
      ;
    return size + i; /* count does not include NUL */
  }

  /* concatenate */
  return Strlcpy(dst+i, src, size-i)+i; /* count does not include NUL */
}

#endif

/*-------------------------------GetTimeval()--------------------------------*/

#if HAVE_GETTIMEOFDAY

void GetTimeval(long *sec, long *usec) {
  struct timeval tv;

  gettimeofday(&tv, NULL);
  *sec = tv.tv_sec;
  *usec = tv.tv_usec;
}

#else
# error GetTimeval() is undefined.
#endif

/*---------------------------------Usleep()----------------------------------*/

#if HAVE_USLEEP

void Usleep(long usec) {
  usleep(usec);
}

#else
# if HAVE_SELECT

void Usleep(long usec) {
  static struct /* 'timeval' */ {
    long tv_sec;
    long tv_usec;
  } delay;
  delay.tv_sec = usec / 1000000L;
  delay.tv_usec = usec % 1000000L;
  select( 0, (void *)0, (void *)0, (void *)0, (void *) &delay );
}

# else
#  if HAVE_POLL

#include <poll.h>
void Usleep(long usec) {
  static int subtotal = 0; /* microseconds */
  int msec; /* milliseconds */
#ifdef pollfd
  /* 'foo' is only here because some versions of 5.3 have
   * a bug where the first argument to poll() is checked
   * for a valid memory address even if the second argument is 0.
   */
  struct pollfd foo;
#endif
  subtotal += usec;
  /* if less then 1 msec request, do nothing but remember it */
  if (subtotal < 1000) return;
  msec = subtotal/1000;
  subtotal = subtotal%1000;
#ifdef pollfd
  poll(&foo,(unsigned long)0,msec);
#else
  poll((void *) 0,(unsigned long)0,msec);
#endif
}

#  else
#   error Usleep() is undefined.
#  endif
# endif
#endif

