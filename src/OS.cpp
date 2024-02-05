#include "OS.h"

#if defined(_WIN32)
// if we compile for Windows native

#include <windows.h>

void OS::LowerThreadPriority()
{
    // ignore errors if this fails
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
}

#elif __has_include(<unistd.h>)
// if we compile for a UNIX'oid

#include <unistd.h>

void OS::LowerThreadPriority()
{
    // we don't really care about errors here, so ignore errno
    (void)!nice(15);
}

#else
// Unsupported OS
#error "Apparently your OS is neither Windows nor appears to be a UNIX variant (no unistd.h). You will have to add support for your OS in src/OS.cpp :/"
#endif
