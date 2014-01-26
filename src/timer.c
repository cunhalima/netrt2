#include <features.h>
//#define __USE_MISC
#define __USE_POSIX199309
#define __USE_XOPEN2K
#include <time.h>
#undef __USE_XOPEN2K
#undef __USE_POSIX199309
//#undef __USE_MISC
#include <stdlib.h>
#include "defs.h"
#if 0
#include <stdlib.h>
#include <sys/time.h>

int sys_getMilli()
{
    int curtime;
    struct timeval tp;
    struct timezone tzp;
    static int secbase = 0;

    gettimeofday(&tp, &tzp);
    if (secbase == 0) {
        secbase = tp.tv_sec;
        return tp.tv_usec / 1000;
    }
    curtime = (tp.tv_sec - secbase) * 1000 + tp.tv_usec / 1000;
    return curtime;
}

void sys_sleep(int milli)
{
    struct timeval t;
    t.tv_sec = milli / 1000;
    t.tv_usec = (milli % 1000) * 1000;
    select(0, NULL, NULL, NULL, &t);
}
#endif

struct timespec res;

int sys_getMilli() {
    int curtime;
    struct timespec tp;
    static time_t secbase = 0;

    if (clock_gettime(CLOCK_REALTIME, &tp) != 0) {
        return 0;
    }
    if (secbase == 0) {
        secbase = tp.tv_sec;
    }
    //con_printf("foi: %d %d\n", ts.tv_sec, ts.tv_nsec);
    curtime = (tp.tv_sec - secbase) * 1000 + tp.tv_nsec / 1000000;
    return curtime;
}

void sys_sleep(int milli) {
    struct timespec tp;

    tp.tv_sec = milli / 1000;
    tp.tv_nsec = (milli % 1000) * 1000000;
    clock_nanosleep(CLOCK_REALTIME, 0, &tp, NULL);
}

void sys_init(void) {
    srand(time(NULL));
}
