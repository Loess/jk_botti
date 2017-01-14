// vi: set ts=4 sw=4 :
// vim: set tw=75 :

// osdep.h - operating system dependencies

/*
 * Copyright (c) 2001 Will Day <willday@hpgx.net>
 *
 *    This file is part of Metamod.
 *
 *    Metamod is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    Metamod is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Metamod; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */

#ifndef OSDEP_H
#define OSDEP_H

// Various differences between WIN32 and Linux.

#include "support_meta.h"	// mBOOL, etc
#include "mreg.h"			// REG_CMD_FN, etc
#include "log_meta.h"		// LOG_ERROR, etc


// String describing platform/DLL-type, for matching lines in metamod.ini.
#ifdef linux
	#define PLATFORM	"linux"
#elif defined(_WIN32)
	#define PLATFORM	"win32"
#else /* unknown */
	#error "OS unrecognized"
#endif /* linux */


// Macro for function-exporting from DLL..
// from SDK dlls/cbase.h:
//! C functions for external declarations that call the appropriate C++ methods

// Windows uses "__declspec(dllexport)" to mark functions in the DLL that
// should be visible/callable externally.
//
// It also apparently requires WINAPI for GiveFnptrsToDll().
//
// See doc/notes_windows_coding for more information..

// Attributes to specify an "exported" function, visible from outside the
// DLL.
#undef DLLEXPORT
#ifdef _WIN32
	#define DLLEXPORT	__declspec(dllexport)
	// WINAPI should be provided in the windows compiler headers.
	// It's usually defined to something like "__stdcall".
#elif defined(linux)
	#define DLLEXPORT	__attribute__ ((visibility ("default")))
	#define WINAPI		/* */
#endif /* _WIN32 */

// Simplified macro for declaring/defining exported DLL functions.  They
// need to be 'extern "C"' so that the C++ compiler enforces parameter
// type-matching, rather than considering routines with mis-matched
// arguments/types to be overloaded functions...
//
// AFAIK, this is os-independent, but it's included here is osdep.h where
// DLLEXPORT is defined, for convenience.
#define C_DLLEXPORT		extern "C" DLLEXPORT


#ifdef _MSC_VER
	// Disable MSVC warning:
	//    4390 : empty controlled statement found; is this what was intended?
	// generated by the RETURN macros.
	#pragma warning(disable: 4390)
#endif /* _MSC_VER */


// Functions & types for DLL open/close/etc operations.
#ifdef linux
	#include <dlfcn.h>
	typedef void* DLHANDLE;
	typedef void* DLFUNC;
	inline DLHANDLE DLOPEN(const char *filename) {
		return(dlopen(filename, RTLD_NOW));
	}
	inline DLFUNC DLSYM(DLHANDLE handle, const char *string) {
		return(dlsym(handle, string));
	}
	inline int DLCLOSE(DLHANDLE handle) {
		return(dlclose(handle));
	}
	inline char* DLERROR(void) {
		return(dlerror());
	}
#elif defined(_WIN32)
	typedef HINSTANCE DLHANDLE;
	typedef FARPROC DLFUNC;
	inline DLHANDLE DLOPEN(const char *filename) {
		return(LoadLibrary(filename));
	}
	inline DLFUNC DLSYM(DLHANDLE handle, const char *string) {
		return(GetProcAddress(handle, string));
	}
	inline int DLCLOSE(DLHANDLE handle) {
		// NOTE: Windows FreeLibrary returns success=nonzero, fail=zero,
		// which is the opposite of the unix convention, thus the '!'.
		return(!FreeLibrary(handle));
	}
	// Windows doesn't provide a function corresponding to dlerror(), so
	// we make our own.
	char *str_GetLastError(void);
	inline char* DLERROR(void) {
		return(str_GetLastError());
	}
#endif /* linux */
const char *DLFNAME(void *memptr);
mBOOL IS_VALID_PTR(void *memptr);


// Attempt to call the given function pointer, without segfaulting.
mBOOL os_safe_call(REG_CMD_FN pfn);


// Windows doesn't have an strtok_r() routine, so we write our own.
#ifdef _WIN32
	#define strtok_r(s, delim, ptrptr)	my_strtok_r(s, delim, ptrptr)
	char *my_strtok_r(char *s, const char *delim, char **ptrptr);
#endif /* _WIN32 */


// Set filename and pathname maximum lengths.  Note some windows compilers
// provide a <limits.h> which is incomplete and/or causes problems; see
// doc/windows_notes.txt for more information.
#ifdef linux
	#include <limits.h>
#elif defined(_WIN32)
	#ifndef NAME_MAX
		#define NAME_MAX	255
	#endif
	#ifndef PATH_MAX
		#define PATH_MAX	255
	#endif
#endif /* linux */


// Various other windows routine differences.
#ifdef linux
	#include <unistd.h>	// sleep
#elif defined(_WIN32)
	#define snprintf	_snprintf
	#define vsnprintf	_vsnprintf
	#define sleep(x)	Sleep(x*1000)
	#define strcasecmp	stricmp
	#define strncasecmp	_strnicmp
#endif /* _WIN32 */

#ifndef S_ISREG
	// Linux gcc defines this; earlier mingw didn't, later mingw does;
	// MSVC doesn't seem to.
	#define S_ISREG(m)	((m) & S_IFREG)
#endif /* not S_ISREG */


// Our handler for new().
//
// Thanks to notes from:
//    http://dragon.klte.hu/~kollarl/C++/node45.html
//
// At one point it appeared MSVC++ was no longer different from gcc, according 
// to:
//    http://msdn.microsoft.com/library/en-us/vclang98/stdlib/info/NEW.asp
//
// However, this page is apparently no longer available from MSDN.  The
// only thing now is:
//    http://msdn.microsoft.com/library/en-us/vccore98/HTML/_crt_malloc.asp
//
// According to Fritz Elfert <felfert@to.com>:
//    set_new_handler() is just a stub which (according to comments in the
//    MSVCRT debugging sources) should never be used. It is just an ugly
//    hack to make STL compile. It does _not_ set the new handler but
//    always calls _set_new_handler(0) instead. _set_new_handler is the
//    "real" function and uses the "old" semantic; handler-type is: 
//       int newhandler(size_t)
//
#ifdef __GNUC__
	void meta_new_handler(void);
#elif defined(_MSC_VER)
	int meta_new_handler(size_t size);
	#define set_new_handler	_set_new_handler
#endif /* _WIN32 */


// Thread handling...
#ifdef linux
	#include <pthread.h>
	typedef	pthread_t 	THREAD_T;
	// returns 0==success, non-zero==failure
	inline int THREAD_CREATE(THREAD_T *tid, void (*func)(void)) {
		int ret;
		ret=pthread_create(tid, NULL, (void *(*)(void*)) func, NULL);
		if(ret != 0) {
			META_ERROR("Failure starting thread: %s", strerror(ret));
			return(ret);
		}
		ret=pthread_detach(*tid);
		if(ret != 0)
			META_ERROR("Failure detaching thread: %s", strerror(ret));
		return(ret);
	}
#elif defined(_WIN32)
	// See:
	//    http://msdn.microsoft.com/library/en-us/dllproc/prothred_4084.asp
	typedef	DWORD 		THREAD_T;
	// returns 0==success, non-zero==failure
	inline int THREAD_CREATE(THREAD_T *tid, void (*func)(void)) {
		HANDLE ret;
		// win32 returns NULL==failure, non-NULL==success
		ret=CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) func, NULL, 0, tid);
		if(ret==NULL)
			META_ERROR("Failure starting thread: %s", str_GetLastError());
		return(ret==NULL);
	}
#endif /* _WIN32 */
#define THREAD_OK	0


// Mutex handling...
#ifdef linux
	typedef pthread_mutex_t		MUTEX_T;
	inline int MUTEX_INIT(MUTEX_T *mutex) {
		int ret;
		ret=pthread_mutex_init(mutex, NULL);
		if(ret!=THREAD_OK)
			META_ERROR("mutex_init failed: %s", strerror(ret));
		return(ret);
	}
	inline int MUTEX_LOCK(MUTEX_T *mutex) {
		int ret;
		ret=pthread_mutex_lock(mutex);
		if(ret!=THREAD_OK)
			META_ERROR("mutex_lock failed: %s", strerror(ret));
		return(ret);
	}
	inline int MUTEX_UNLOCK(MUTEX_T *mutex) {
		int ret;
		ret=pthread_mutex_unlock(mutex);
		if(ret!=THREAD_OK)
			META_ERROR("mutex_unlock failed: %s", strerror(ret));
		return(ret);
	}
#elif defined(_WIN32)
	// Win32 has "mutexes" as well, but CS's are simpler.
	// See:
	//    http://msdn.microsoft.com/library/en-us/dllproc/synchro_2a2b.asp
	typedef CRITICAL_SECTION	MUTEX_T;
	// Note win32 routines don't return any error (return void).
	inline int MUTEX_INIT(MUTEX_T *mutex) {
		InitializeCriticalSection(mutex);
		return(THREAD_OK);
	}
	inline int MUTEX_LOCK(MUTEX_T *mutex) {
		EnterCriticalSection(mutex);
		return(THREAD_OK);
	}
	inline int MUTEX_UNLOCK(MUTEX_T *mutex) {
		LeaveCriticalSection(mutex);
		return(THREAD_OK);
	}
#endif /* _WIN32 (mutex) */


// Condition variables...
#ifdef linux
	typedef pthread_cond_t	COND_T;
	inline int COND_INIT(COND_T *cond) {
		int ret;
		ret=pthread_cond_init(cond, NULL);
		if(ret!=THREAD_OK)
			META_ERROR("cond_init failed: %s", strerror(ret));
		return(ret);
	}
	inline int COND_WAIT(COND_T *cond, MUTEX_T *mutex) {
		int ret;
		ret=pthread_cond_wait(cond, mutex);
		if(ret!=THREAD_OK)
			META_ERROR("cond_wait failed: %s", strerror(ret));
		return(ret);
	}
	inline int COND_SIGNAL(COND_T *cond) {
		int ret;
		ret=pthread_cond_signal(cond);
		if(ret!=THREAD_OK)
			META_ERROR("cond_signal failed: %s", strerror(ret));
		return(ret);
	}
#elif defined(_WIN32)
	// Since win32 doesn't provide condition-variables, we have to model
	// them with mutex/critical-sections and win32 events.  This uses the
	// second (SetEvent) solution from:
	//
	//    http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
	//
	// but without the waiters_count overhead, since we don't need
	// broadcast functionality anyway.  Or actually, I guess it's more like
	// the first (PulseEvent) solution, but with SetEven rather than
	// PulseEvent. :)
	//
	// See also:
	//    http://msdn.microsoft.com/library/en-us/dllproc/synchro_8ann.asp
	typedef HANDLE COND_T; 
	inline int COND_INIT(COND_T *cond) {
		*cond = CreateEvent(NULL,	// security attributes (none)
							FALSE,	// manual-reset type (false==auto-reset)
							FALSE,	// initial state (unsignaled)
							NULL);	// object name (unnamed)
		// returns NULL on error
		if(*cond==NULL) {
			META_ERROR("cond_init failed: %s", str_GetLastError());
			return(-1);
		}
		else
			return(0);
	}
	inline int COND_WAIT(COND_T *cond, MUTEX_T *mutex) {
		DWORD ret;
		LeaveCriticalSection(mutex);
		ret=WaitForSingleObject(*cond, INFINITE);
		EnterCriticalSection(mutex);
		// returns WAIT_OBJECT_0 if object was signaled; other return
		// values indicate errors.
		if(ret == WAIT_OBJECT_0)
			return(0);
		else {
			META_ERROR("cond_wait failed: %s", str_GetLastError());
			return(-1);
		}
	}
	inline int COND_SIGNAL(COND_T *cond) {
		BOOL ret;
		ret=SetEvent(*cond);
		// returns zero on failure
		if(ret==0) {
			META_ERROR("cond_signal failed: %s", str_GetLastError());
			return(-1);
		}
		else
			return(0);
	}
#endif /* _WIN32 (condition variable) */


#endif /* OSDEP_H */
