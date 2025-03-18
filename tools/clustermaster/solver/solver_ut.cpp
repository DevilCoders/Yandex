#include "batch.h"
#include "request.h"
#include "solve.h"
#include "solver.h"
#include "thingy.h"

#include <tools/clustermaster/communism/core/core.h>
#include <tools/clustermaster/communism/util/daemon.h>
#include <tools/clustermaster/communism/util/dirut.h>
#include <tools/clustermaster/communism/util/file_reopener_by_signal.h>
#include <tools/clustermaster/communism/util/pidfile.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/svnversion/svnversion.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/memory/tempbuf.h>
#include <util/stream/file.h>
#include <util/stream/input.h>
#include <util/system/daemon.h>
#include <util/system/file.h>
#include <util/system/info.h>
#include <util/system/sigset.h>
#include <util/system/spinlock.h>
#include <util/thread/pool.h>

#ifdef _unix_
#   include <netinet/tcp.h>

#   ifdef __FreeBSD__
#       ifndef TCP_KEEPIDLE
#           define TCP_KEEPIDLE 0x100
//          warning TCP_KEEPIDLE is not defined. Using compatibility hack, please switch to FreeBSD 9+.
#       endif
#       ifndef TCP_KEEPINTVL
#           define TCP_KEEPINTVL 0x200
//          warning TCP_KEEPINTVL is not defined. Using compatibility hack, please switch to FreeBSD 9+.
#       endif
#   endif
#endif

typedef TLogOutput Log;

namespace NGlobal {

TAtomic NeedSolve = false;
TKeyMapper KeyMapper;
TKnownLimits KnownLimits;
TAtomic LastRequestIndexNumber = 0;
THolder<TFileReopenerBySignal> LogFileReopener;

};

using namespace NCommunism;

int TRequestBuilderInstanceCounter = 0;
class TRequestBuilder: public TRequest {
    public:
        TRequestBuilder()
            : TRequest(nullptr, ++TRequestBuilderInstanceCounter) { }

        void Insert(const TString &key, double ration) {
            operator[](NGlobal::KeyMapper[key]) += (ration >= 1.0 ? Max<unsigned>() : (unsigned) (Max<unsigned>() * ration));
        }

        size_t Size() const {
            return size();
        }
};
