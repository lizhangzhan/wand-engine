#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WinSock2.h>

#ifndef EPOCHFILETIME
# define EPOCHFILETIME (116444736000000000i64)
#endif

int gettimeofday(struct timeval * tv, void * tz) {
    FILETIME        ft;
    LARGE_INTEGER   li;
    __int64         t;

    if (tv == NULL)
        return 0;

    GetSystemTimeAsFileTime(&ft);

    li.LowPart  = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    t  = li.QuadPart;       /* In 100-nanosecond intervals */
    t -= EPOCHFILETIME;     /* Offset to the Epoch time */
    t /= 10;                /* In microseconds */
    tv->tv_sec  = (long)(t / 1000000);
    tv->tv_usec = (long)(t % 1000000);
    return 0;
}
