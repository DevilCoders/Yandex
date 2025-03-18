#include "data.h"

using namespace NZooKeeper;

// detect future ABI changes in zookeeper C library
static_assert(sizeof(TId) == sizeof(char*) * 2, "expect sizeof(TId) == sizeof(char*) * 2");
static_assert(sizeof(TACL) == sizeof(char*) * 3, "expect sizeof(TACL) == sizeof(char*) * 3");

#ifdef __x86_64__
static_assert(sizeof(TStat) == 72, "expect sizeof(TStat) == 72");
#endif

#ifdef __i386__
static_assert(sizeof(TStat) == 68, "expect sizeof(TStat) == 68");
#endif

TString TStat::ToString() const {
    return Sprintf("Stat czxid:%" PRId64 " mzxid:%" PRId64 " ctime:%" PRId64 " mtime:%" PRId64 " version:%d"
                   " cversion:%d aversion:%d ephemeralOwner:%" PRId64 "dataLength:%d numChildren:%d pzxid:%" PRId64,
                   (ui64)czxid, (ui64)mzxid, (ui64)ctime, (ui64)mtime, version, cversion, aversion, (ui64)ephemeralOwner, dataLength, numChildren, (ui64)pzxid);
}
