#include "qd_registry.h"
#include "qd_reporters.h"

#include <util/generic/singleton.h>

namespace NQueryData {

    static void DoInsertSource(TSources& res, const TSourcePtr& pp) {
        if (!!pp) {
            res.push_back(pp);
        }
    }

    static void DoInsertSource(TTotalSize& res, const TSourcePtr& pp) {
        if (!!pp) {
            res.AllSize += pp->ApproxLoadedSize(false);
            res.RAMSize += pp->ApproxLoadedSize(true);
            switch (pp->GetLoadMode()) {
            default:
                res.InvalidCount++;
                break;
            case LM_RAM:
                res.RAMCount++;
                break;
            case LM_FAST_MMAP:
                res.FastMMapCount++;
                break;
            case LM_MMAP:
                res.MMapCount++;
                break;
            }
        } else {
            res.InvalidCount++;
        }
    }

    template <typename TRes, typename TPtr>
    static void DoAddSource(TRes& res, const TPtr& p) {
        DoInsertSource(res, !p ? nullptr : p->GetObject());
    }

    template <typename TRes, typename TColl>
    static void DoGetSources(TRes& res, const TGuarded<TColl>& sources) {
        if (!sources.Unsafe().empty()) {
            typename TGuarded<TColl>::TRead acc(sources);
            for (const auto& item : acc.Object) {
                DoAddSource(res, item);
            }
        }
    }

    template <typename TRes>
    static void DoGetSourceLists(TRes& res, const TSourceListsG& lists) {
        if (!lists.Unsafe().empty()) {
            TSourceListsG::TRead acc(lists);
            for (const auto& srcList : acc.Object) {
                if (!srcList) {
                    continue;
                }

                TSourceList::TSharedObjectPtr p = srcList->GetObject();
                if (!p) {
                    continue;
                }

                for (const auto& regItem : *p) {
                    DoAddSource(res, regItem.second);
                }
            }
        }
    }

    void TSourcesRegistry::GetSources(TSources& src) const {
        src.clear();
        DoGetSources(src, FakeSources);
        DoGetSources(src, StaticSources);
        DoGetSourceLists(src, SourceLists);
    }

    TTotalSize TSourcesRegistry::TotalSize() const {
        TTotalSize tot;
        DoGetSources(tot, FakeSources);
        DoGetSources(tot, StaticSources);
        DoGetSourceLists(tot, SourceLists);
        return tot;
    }

    NSc::TValue TSourcesRegistry::GetStats(EStatsVerbosity sv) const {
        NSc::TValue res;

        try {
            if (!FakeSources.Unsafe().empty()) {
                NSc::TValue& v = res["fakes"];
                TFakeSourceUpdatersG::TRead acc(FakeSources);
                for (ui32 i = 0, sz = acc.Object.size(); i < sz; ++i) {
                    TFakeSourceUpdaterPtr p = acc.Object[i];

                    if (!p) {
                        ReportNullUpdater(res, "fake", i);
                    } else {
                        NSc::TValue vv = NextReportNode(v, "fake");
                        ReportSourceStats(vv, sv, p.Get());
                    }
                }
            }

            if (!StaticSources.Unsafe().empty()) {
                NSc::TValue& v = res["sources"];
                TSourceCheckersG::TRead acc(StaticSources);
                for (ui32 i = 0, sz = acc.Object.size(); i < sz; ++i) {
                    TSourceCheckerPtr p = acc.Object[i];

                    if (!p) {
                        ReportNullUpdater(res, "static source", i);
                    } else {
                        NSc::TValue vv = NextReportNode(v, p->GetMonitorFileName());
                        ReportRealSourceStats(vv, sv, p.Get());
                    }
                }
            }

            if (!SourceLists.Unsafe().empty()) {
                NSc::TValue& v = res["lists"];
                TSourceListsG::TRead acc(SourceLists);
                for (ui32 i = 0, sz = acc.Object.size(); i < sz; ++i) {
                    TSourceListPtr lstp = acc.Object[i];

                    if (!lstp) {
                        ReportNullUpdater(res, "source list", i);
                        continue;
                    }

                    TString fname = lstp->GetMonitorFileName();
                    NSc::TValue lstv = NextReportNode(v, fname);

                    try {
                        ReportRealFile(lstv, fname);
                    } catch (const yexception& e) {
                        ReportError(lstv, e.what());
                    }

                    TSourceList::TSharedObjectPtr lsto = lstp->GetObject();

                    if (!lsto) {
                        ReportError(lstv, "source list is not instantiated");
                        continue;
                    }

                    ui32 j = 0;
                    for (const auto& item : *lsto) {
                        TSourceList::TItemCheckerPtr p = item.second;
                        if (!p) {
                            ReportNullUpdater(lstv, "source", j, item.first);
                        } else {
                            NSc::TValue vv = NextReportNode(lstv["entries"], item.first);
                            ReportRealSourceStats(vv, sv, p.Get());
                        }
                        ++j;
                    }
                }
            }

            TTotalSize tot = TotalSize();
            res["stats"]["total-size"] = tot.AllSize;
            res["stats"]["total-ram-size"] = tot.RAMSize;
            res["stats"]["tries-count-ram"] = tot.RAMCount;
            res["stats"]["tries-count-fast-mmap"] = tot.FastMMapCount;
            res["stats"]["tries-count-mmap"] = tot.MMapCount;
            res["stats"]["tries-count-invalid"] = tot.InvalidCount;
            res["stats"]["tries-count-all"] = tot.AllCount();
            res["stats"]["memory-usage"] = CurrentMemoryUsage();
            res["stats"]["max-memory-usage"] = -1;
            res["stats"]["max-total-ram-size"] = (i64)SourceOpts.MaxTotalRAMSize;
            res["stats"]["max-trie-ram-size"] = (i64)SourceOpts.MaxTrieRAMSize;
            res["stats"]["min-trie-fast-mmap-size"] = (i64)SourceOpts.MinTrieFastMMapSize;
            res["stats"]["enable-fast-mmap"] = SourceOpts.EnableFastMMap;
            res["stats"]["enable-debug-info"] = SourceOpts.EnableDebugInfo;
        } catch (const yexception& e) {
            ReportError(res, e.what());
        }

        return res;
    }

}
