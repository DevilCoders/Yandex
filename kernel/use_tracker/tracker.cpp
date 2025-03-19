#include "tracker.h"

using namespace NThreading;

namespace NUseTracker {

TFuture<void> ShouldBeSet(TFuture<void> future, TString message) {
    auto enabled = MakeAtomicShared<TAtomic>(1);
    auto guard = DoOnDestroy([=]() {
                Y_VERIFY(AtomicGet(*enabled) != 1, "%s", message.data());
            });
    return future.Apply([enabled, guard] (TFuture<void>) {
            AtomicSet(*enabled, 0);
        });
}

}
