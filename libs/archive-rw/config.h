#ifndef HAD_CONFIG_H
#define HAD_CONFIG_H
#ifndef HAD_ZIPCONF_H
#include "zipconf.h"
#endif
/* BEGIN DEFINES */
/* #undef HAVE___PROGNAME */
/* #undef HAVE__CHMOD */
/* #undef HAVE__CLOSE */
/* #undef HAVE__DUP */
/* #undef HAVE__FDOPEN */
/* #undef HAVE__FILENO */
/* #undef HAVE__OPEN */
/* #undef HAVE__SETMODE */
/* #undef HAVE__SNPRINTF */
/* #undef HAVE__STRDUP */
/* #undef HAVE__STRICMP */
/* #undef HAVE__STRTOI64 */
/* #undef HAVE__STRTOUI64 */
/* #undef HAVE__UMASK */
/* #undef HAVE__UNLINK */
/* #undef HAVE_CLONEFILE */
/* #undef HAVE_COMMONCRYPTO */
/* #undef HAVE_CRYPTO */
#define HAVE_FICLONERANGE
#define HAVE_FILENO
#define HAVE_FSEEKO
#define HAVE_FTELLO
/* #undef HAVE_GETPROGNAME */
/* #undef HAVE_GNUTLS */
/* #undef HAVE_LIBBZ2 */
#define HAVE_OPEN
/* #undef HAVE_OPENSSL */
#define HAVE_MKSTEMP
/* #undef HAVE_SETMODE */
#define HAVE_SNPRINTF
#define HAVE_SSIZE_T_LIBZIP
#define HAVE_STRCASECMP
#define HAVE_STRDUP
/* #undef HAVE_STRICMP */
#define HAVE_STRTOLL
#define HAVE_STRTOULL
/* #undef HAVE_STRUCT_TM_TM_ZONE */
#define HAVE_STDBOOL_H
#define HAVE_STRINGS_H
#define HAVE_UNISTD_H
/* #undef __INT8_LIBZIP */
#define INT8_T_LIBZIP 1
#define UINT8_T_LIBZIP 1
/* #undef __INT16_LIBZIP */
#define INT16_T_LIBZIP 2
#define UINT16_T_LIBZIP 2
/* #undef __INT32_LIBZIP */
#define INT32_T_LIBZIP 4
#define UINT32_T_LIBZIP 4
/* #undef __INT64_LIBZIP */
#define INT64_T_LIBZIP 8
#define UINT64_T_LIBZIP 8
#define SHORT_LIBZIP 2
#define INT_LIBZIP 4
#define LONG_LIBZIP 8
#define LONG_LONG_LIBZIP 8
#define SIZEOF_OFF_T 8
#define SIZE_T_LIBZIP 8
#define SSIZE_T_LIBZIP 8
/* #undef HAVE_DIRENT_H */
/* #undef HAVE_FTS_H */
/* #undef HAVE_NDIR_H */
/* #undef HAVE_SYS_DIR_H */
/* #undef HAVE_SYS_NDIR_H */
/* #undef WORDS_BIGENDIAN */
#define HAVE_SHARED
/* END DEFINES */
#define PACKAGE "libzip"
#define VERSION "1.5.1a"

#ifndef HAVE_SSIZE_T_LIBZIP
#  if SIZE_T_LIBZIP == INT_LIBZIP
typedef int ssize_t;
#  elif SIZE_T_LIBZIP == LONG_LIBZIP
typedef long ssize_t;
#  elif SIZE_T_LIBZIP == LONG_LONG_LIBZIP
typedef long long ssize_t;
#  else
#error no suitable type for ssize_t found
#  endif
#endif

#endif /* HAD_CONFIG_H */
