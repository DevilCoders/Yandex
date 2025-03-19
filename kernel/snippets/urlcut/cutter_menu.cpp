#include "cutter_menu.h"

#include "consts.h"
#include "tokens.h"
#include "hilited_string.h"

#include <util/generic/list.h>
#include <util/generic/hash_set.h>
#include <utility>

namespace NUrlCutter {
    class TMenuCutter::TMenuCutterImpl {
    private:
        typedef TList<TTokenInfo>::iterator It;
        typedef std::pair<It, It> TBorders;

        TTokenList& Tokens;
        const i32 MaxLen;

    private:
        TBorders FindLastPart() {
            It beg = Tokens.begin();
            for (It it = Tokens.begin(); it != Tokens.end(); ++it) {
                It nit = it;
                nit++;
                if (nit == Tokens.end() || nit->PType == PT_CGI_SP) {
                    return {beg, nit};
                }
                if (it->PType == PT_PATH_SP) {
                    beg = it;
                }
            }
            return {beg, Tokens.end()};
        }

        i64 CalcWeight(const It& beg, const It& end) {
            i64 weight = 0;
            THashSet<TUtf16String> unique;
            for (It it = beg; it != end; ++it) {
                weight += it->GetFreq(unique, false);
                it->FillUsed(unique);
            }
            return weight;
        }

        TBorders FindBest() {
            i64 bestWeight = -1;
            TBorders best(Tokens.begin(), Tokens.begin());
            for (It beg = Tokens.begin(); beg != Tokens.end(); ++beg) {
                i64 localBestWeight = -1;
                TBorders localBest(Tokens.begin(), Tokens.begin());
                It end = beg;
                int len = (beg == Tokens.begin() ? 0 : (int)Ellipsis.size());
                bool hasNotNone = false;
                for (It it = beg; it != Tokens.end(); ++it) {
                    It nit = it;
                    nit++;
                    len += (int)it->Len;
                    hasNotNone |= (it->TType != TT_NONE);
                    int nLen = len + (nit == Tokens.end() ? 0 : (int)Ellipsis.size());
                    if (hasNotNone && nLen <= MaxLen) {
                        end = nit;
                        hasNotNone = false;
                        if (beg != end) {
                            i64 weight = CalcWeight(beg, end);
                            if (localBestWeight <= weight) {
                                localBestWeight = weight;
                                localBest = {beg, end};
                            }
                        }
                    }
                }
                if (bestWeight < localBestWeight) {
                    bestWeight = localBestWeight;
                    best = localBest;
                }
            }
            return best;
        }

    public:
        TMenuCutterImpl(TTokenList& tokens, i32 maxLen)
            : Tokens(tokens)
            , MaxLen(maxLen)
        {
            for (It it = Tokens.begin(); it != Tokens.end(); ++it)
                it->AddRevFreq = 0;

            TBorders borders = FindLastPart();
            for (It it = borders.first; it != borders.second; ++it)
                it->AddRevFreq += 1000;
        }

        THilitedString GetUrl() {
            if (Tokens.empty())
                return THilitedString();

            THilitedString url;
            TBorders b = FindBest();
            if (b.first != Tokens.begin()) {
                url.Append(Ellipsis);
            }
            for (It it = b.first; it != b.second; ++it) {
                url.Append(it->GetHilitedToken());
            }
            if (b.second != Tokens.end()) {
                url.Append(Ellipsis);
            }
            return url;
        }
    };

    TMenuCutter::TMenuCutter(TTokenList& tokens, i32 maxLen)
        : Impl(new TMenuCutterImpl(tokens, maxLen))
    {
    }

    TMenuCutter::~TMenuCutter() {
    }

    THilitedString TMenuCutter::GetUrl() {
        return Impl->GetUrl();
    }
}
