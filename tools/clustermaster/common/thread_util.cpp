#if defined(__FreeBSD__)

#include <pthread.h>
#include <pthread_np.h>

void SetCurrentThreadName(const char *name)
{
    pthread_set_name_np(pthread_self(), name);
}

#else

void SetCurrentThreadName(const char*)
{
}

#endif
