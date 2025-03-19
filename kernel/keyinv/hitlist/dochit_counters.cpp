#include "dochit_counters.h"

bool TDocHitCounters::Enable_ = false;
TVector<TDocHitCounters::atomic_ui32> TDocHitCounters::DocsPopularity_ = {};
