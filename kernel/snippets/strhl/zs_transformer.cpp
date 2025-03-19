#include "zs_transformer.h"
#include "zonedstring.h"

namespace NSnippets {
    TZonedStringTransformer::TZonedStringTransformer(TZonedString& z, bool skipZeroLenSpans)
        : Z(z)
        , NewBeg(Z.String.size(), -1)
        , NewEnd(Z.String.size(), -1)
        , CurIdx(0)
        , SkipZeroLenSpans(skipZeroLenSpans)
    {
    }

    void TZonedStringTransformer::Step(int cnt) {
        for (int i = 0; i < cnt; ++i, ++CurIdx) {
            NewBeg[CurIdx] = NewEnd[CurIdx] = S.size();
            S += Z.String[CurIdx];
        }
    }

    void TZonedStringTransformer::Delete(int cnt) {
        CurIdx += cnt;
    }

    void TZonedStringTransformer::Replace(const TUtf16String& x) {
        NewBeg[CurIdx] = S.size();
        NewEnd[CurIdx] = S.size() + x.size() - 1;
        S += x;
        ++CurIdx;
    }

    void TZonedStringTransformer::InsertBeforeNext(const TUtf16String& x) {
        S += x;
    }

    void TZonedStringTransformer::Complete() {
        Step(Z.String.size() - CurIdx);
        for (size_t i = 0; i + 1 < NewEnd.size(); ++i) {
            if (NewEnd[i + 1] == -1) {
                NewEnd[i + 1] = NewEnd[i];
            }
        }
        for (size_t i = NewBeg.size(); i > 1; --i) {
            if (NewBeg[i - 2] == -1) {
                NewBeg[i - 2] = NewBeg[i - 1];
            }
        }
        const TWtringBuf old = Z.String;
        Z.String = S;
        for (TZonedString::TZones::iterator it = Z.Zones.begin(); it != Z.Zones.end(); ++it) {
            size_t n = 0;
            for (size_t i = 0; i < it->second.Spans.size(); ++i) {
                TWtringBuf s = it->second.Spans[i].Span;
                ssize_t b = s.data() - old.data();
                if (b < 0) {
                    b = 0;
                }
                ssize_t e = (s.data() + s.size()) - old.data();
                if (e > (ssize_t)old.size()) {
                    e = old.size();
                }
                if (e == b && !SkipZeroLenSpans) {
                    if (NewBeg[b] == -1) {
                        continue;
                    }
                } else if (e <= b || NewBeg[b] == -1 || NewEnd[e - 1] == -1) {
                    continue;
                }
                DoSwap(it->second.Spans[n], it->second.Spans[i]);
                it->second.Spans[n++].Span = (e == b) ?
                    TWtringBuf(Z.String.data() + NewBeg[b], (size_t)0) :
                    TWtringBuf(Z.String.data() + NewBeg[b], Z.String.data() + NewEnd[e - 1] + 1);
            }
            it->second.Spans.resize(n);
        }
    }
}
