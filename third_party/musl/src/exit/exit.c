#include <stdlib.h>
#include <stdint.h>
#include "libc.h"

static void dummy()
{
}

// XXX: Workaround broken handling of weak_alias, remove in future!
// This is a workaround for incorrect weak alias handling in our JIT.
//
// Always use full definitions for these, don't bother with aliases.
// This should be fine (safe/correct) functionally but causes portions of musl
// to be included when they could possibly be omitted.
//
// IIRC newer LLVM versions can handle the weak alias resolution as desired,
// mostly by having notions of search scope and not just using first "definition".
// Anyway, when moving to new versions try dropping this in favor of vanilla musl!
// 
// New behavior of macro:
// Declare second argument using type of first
#define weak_alias_workaround(old, new) extern hidden __typeof(old) new

/* atexit.c and __stdio_exit.c override these. the latter is linked
 * as a consequence of linking either __toread.c or __towrite.c. */
weak_alias_workaround(dummy, __funcs_on_exit);
weak_alias_workaround(dummy, __stdio_exit);
weak_alias(dummy, _fini);

extern weak hidden void (*const __fini_array_start)(void), (*const __fini_array_end)(void);

static void libc_exit_fini(void)
{
	uintptr_t a = (uintptr_t)&__fini_array_end;
	for (; a>(uintptr_t)&__fini_array_start; a-=sizeof(void(*)()))
		(*(void (**)())(a-sizeof(void(*)())))();
	_fini();
}

weak_alias(libc_exit_fini, __libc_exit_fini);

_Noreturn void exit(int code)
{
	__funcs_on_exit();
	__libc_exit_fini();
	__stdio_exit();
	_Exit(code);
}
