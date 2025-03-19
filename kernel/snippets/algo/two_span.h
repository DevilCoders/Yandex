#pragma once

#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/sent_match/callback.h>
#include <kernel/snippets/weight/weighter_decl.h>

#include <util/generic/bitmap.h>
#include <util/generic/vector.h>

namespace NSnippets
{
    class TConfig;
    class TUniSpanIter;
    struct ISnippetCandidateDebugHandler;
    class TSentsMatchInfo;
    class TSnip;

    namespace NSnipWordSpans
    {
        template<class T>
        inline TDynBitMap GenUsefullWordsBSet(const TVector<T> &seenLikePos, const TQueryy& query) {
            TDynBitMap ret;
            ret.Reserve(query.PosCount());
            for (size_t pos = 0; pos < seenLikePos.size(); ++pos) {
                if (seenLikePos[pos] && query.Positions[pos].IsUserWord && !(query.Positions[pos].IsStopWord || query.Positions[pos].IsConjunction)) {
                    ret.Set(pos);
                }
            }
            return ret;
        }

        class TGetTwoPlusSnip {
        private:
            class TImpl;
            THolder<TImpl> Impl;

        public:
            TGetTwoPlusSnip(const TConfig& cfg, TMxNetWeighter& weighter, TUniSpanIter &spanSet, const TSentsMatchInfo &sMInfo,
                ECandidateSource source, ISnippetCandidateDebugHandler *callback, bool dontGrow, bool needCheckTitle, float repeatedTitlePessimizeFactor);
            ~TGetTwoPlusSnip();
            TSnip GetSnip(float maxSize, float maxPartSize, size_t fragCnt, const TVector<TDynBitMap> &snipBitSet);
        };

    }
}
