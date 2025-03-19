#include "order.h"
#include "view.h"
#include <library/cpp/charset/wide.h>

namespace NSnippets {
    void DumpResultCopy(const TSentsOrder& all, TVector<TUtf16String>& res) {
        for (TSentsRangeList::TConstIterator i = all.Sents.Begin(); i != all.Sents.End(); ++i) {
            for (TArchiveSentList::TConstIterator j = i->ResultBeg; ; ++j) {
                if (j.Item()) {
                    res.push_back(TUtf16String(j->Sent.data(), j->Sent.size()));
                }
                if (j == i->ResultEnd) {
                    break;
                }
            }
        }
    }
    void DumpResultCopy(const TSentsOrder& all, TVector< TVector<TUtf16String> >& res) {
        res.reserve(all.Sents.Size());
        for (TSentsRangeList::TConstIterator i = all.Sents.Begin(); i != all.Sents.End(); ++i) {
            res.push_back(TVector<TUtf16String>());
            for (TArchiveSentList::TConstIterator j = i->ResultBeg; ; ++j) {
                if (j.Item()) {
                    res.back().push_back(TUtf16String(j->Sent.data(), j->Sent.size()));
                }
                if (j == i->ResultEnd) {
                    break;
                }
            }
        }
    }
    void DumpIds(const TSentsOrder& all, TVector<int>& res) {
        for (TSentsRangeList::TConstIterator i = all.Sents.Begin(); i != all.Sents.End(); ++i) {
            for (TArchiveSentList::TConstIterator j = i->ResultBeg; ; ++j) {
                if (j.Item()) {
                    res.push_back(j->SentId);
                }
                if (j == i->ResultEnd) {
                    break;
                }
            }
        }
    }
    void DumpAttrs(const TSentsOrder& all, TVector<TString>& res) {
        for (TSentsRangeList::TConstIterator i = all.Sents.Begin(); i != all.Sents.End(); ++i) {
            for (TArchiveSentList::TConstIterator j = i->ResultBeg; ; ++j) {
                if (j.Item()) {
                    res.push_back(WideToChar(TUtf16String(j->Attr.data(), j->Attr.size()), CODES_YANDEX));
                }
                if (j == i->ResultEnd) {
                    break;
                }
            }
        }
    }
    void DumpResult(const TSentsOrder& all, TArchiveView& res, bool allowEmptySent) {
        for (TSentsRangeList::TConstIterator i = all.Sents.Begin(); i != all.Sents.End(); ++i) {
            for (TArchiveSentList::TConstIterator j = i->ResultBeg; ; ++j) {
                if (j.Item()) {
                    if (allowEmptySent || !j->Sent.empty()) {
                        res.PushBack(&*j);
                    }
                }
                if (j == i->ResultEnd) {
                    break;
                }
            }
        }
    }
    void DumpResult(const TSentsOrder& all, TArchiveViews& res, bool allowEmptySent) {
        for (TSentsRangeList::TConstIterator i = all.Sents.Begin(); i != all.Sents.End(); ++i) {
            res.Views.push_back(TArchiveView());
            for (TArchiveSentList::TConstIterator j = i->ResultBeg; ; ++j) {
                if (j.Item()) {
                    if (allowEmptySent || !j->Sent.empty()) {
                        res.Views.back().PushBack(&*j);
                    }
                }
                if (j == i->ResultEnd) {
                    break;
                }
            }
            if (res.Views.back().Empty()) {
                res.Views.pop_back();
            }
        }
    }
    void TSentsOrderGenerator::Cutoff(const TSentsOrder& ex, TSentsOrder& result) {
        TSentsRangeList::TConstIterator e = ex.Sents.Begin();
        TSentsRangeList::TIterator i = result.Sents.Begin();
        while (e != ex.Sents.End() && i != result.Sents.End()) {
            if (e->LastId < i->FirstId) {
                ++e;
            } else if (e->FirstId > i->LastId) {
                ++i;
            } else {
                TSentsRangeList::TIterator ni = i;
                ++ni;
                i->Unlink();
                THolder<TSentsRange> harakiri(i.operator->());
                if (i->FirstId < e->FirstId) {
                    THolder<TSentsRange> l(new TSentsRange());
                    l->FirstId = i->FirstId;
                    l->LastId = e->FirstId - 1;
                    l.Release()->LinkBefore(*ni);
                }
                if (i->LastId > e->LastId) {
                    THolder<TSentsRange> r(new TSentsRange());
                    r->FirstId = e->LastId + 1;
                    r->LastId = i->LastId;
                    r.Release()->LinkBefore(*ni);
                }
                i = ni;
            }
        }
    }
}
