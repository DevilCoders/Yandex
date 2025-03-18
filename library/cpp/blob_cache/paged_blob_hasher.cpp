#include "paged_blob_hasher.h"

template <>
void Out<THasherStats>(IOutputStream& os, const THasherStats& stats) {
    os << "NumHit=" << stats.NumHit << " "
       << "NumMissLock=" << stats.NumMissLock << " "
       << "NumMissCold=" << stats.NumMissCold << " "
       << "NumMissHot=" << stats.NumMissHot << " "
       << "NumFlushConflict=" << stats.NumFlushConflict << " "
       << "NumStore=" << stats.NumStore << " "
       << "NumStoreMiss=" << stats.NumStoreMiss << " "
       << "NumRefresh=" << stats.NumRefresh << " "
       << "NumSkipRefresh=" << stats.NumSkipRefresh << " "
       << "NumOverflowFail=" << stats.NumOverflowFail << " "
       << "NumPageFail=" << stats.NumPageFail << " "
       << "NumPageFlush=" << stats.NumPageFlush << " "
       << "HasherSize=" << stats.HasherSize << " "
       << "PairsSizeFrac=" << stats.PairsSizeFrac << " "
       << "PagesSizeFrac=" << stats.PagesSizeFrac << " "
       << "BytesSizeFrac=" << stats.BytesSizeFrac << " "
       << "SlotsSizeFrac=" << stats.SlotsSizeFrac << " "
       << "BytesUtilization=" << stats.BytesUtilization << " "
       << "SlotsUtilization=" << stats.SlotsUtilization << " "
       << "BytesWaste=" << stats.BytesWaste << " "
       << "SlotsWaste=" << stats.SlotsWaste << " "
       << "NumSkippedPages=" << stats.NumSkippedPages;
}
