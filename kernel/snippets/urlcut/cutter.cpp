#include "cutter.h"

#include "consts.h"
#include "tokens.h"
#include "hilited_string.h"

#include <util/generic/list.h>
#include <util/generic/hash_set.h>
#include <utility>

namespace NUrlCutter {
    class TUrlCutter::TUrlCutterImpl {
    private:
        typedef TTokenList::iterator It;
        typedef TTokenList::reverse_iterator RIt;

        TTokenList& Tokens;
        i32 MaxLen;
        i32 ThresholdLen;
        const NSnippets::W2WTrie& DomainTrie;
        THashSet<TUtf16String> Used;

    public:
        TUrlCutterImpl(TTokenList& tokens, i32 maxLen, i32 thresholdLen, const NSnippets::W2WTrie& domainTrie)
            : Tokens(tokens)
            , MaxLen(maxLen)
            , ThresholdLen(thresholdLen)
            , DomainTrie(domainTrie)
            , Used()
        {
        }

        template <class DIt>
        inline i32 CalcLen(i32 len, DIt& it, DIt& edge) {
            i32 res = len + (i32)it->Len;
            //если не дошли до начала, то резервируем место под многоточие в начале
            if (it != edge) {
                res += (i32)Ellipsis.size();
            }
            return res;
        }

        template <class DIt>
        i32 SimpleAppend(THilitedString& url, DIt begin, DIt end, i32& maxLen, bool reverse, bool& ee) {
            i32 len(0);
            //идем от конца домена к началу и добавляем слова, пока под них есть место
            DIt edge = end;
            --edge;
            for (DIt it(begin); it != end; ++it) {
                if (CalcLen(len, it, edge) <= maxLen) {
                    if (reverse) {
                        url = it->GetHilitedToken().Append(url);
                    } else {
                        url.Append(it->GetHilitedToken());
                    }
                    len += (i32)it->Len;
                    it->SetStatus(TS_USED, Used);
                    ee = false;
                } else {
                    if (!ee) {
                        if (reverse) {
                            url = THilitedString(Ellipsis).Append(url);
                        } else {
                            url.Append(Ellipsis);
                        }
                        len += (i32)Ellipsis.size();
                        ee = true;
                    }
                    break;
                }
            }
            return len;
        }

        inline i32 ProcessDomain(THilitedString& url, i32 maxLen) {
            //ищем границу домена
            It end = Tokens.begin();

            if (Tokens.size() == 0 || end->PType != PT_DOMAIN)
                return 0;

            for (; end != Tokens.end() && end->PType != PT_PATH_SP; ++end) {
            }

            //если урл состоит не только из доменa, то резервирум место
            if (end != Tokens.end()) {
                size_t endLen(0);
                for (It it = end; it != Tokens.end(); ++it) {
                    endLen += it->Len;
                }
                if (endLen == 1) {
                    // остался только никому не нужный слэш
                    end = Tokens.erase(end);
                    endLen = 0;
                } else {
                    endLen -= end->Len;
                    ++end;
                }
                //если текста не в домене мало
                maxLen -= endLen < Ellipsis.size() ? (i32)endLen : (i32)Ellipsis.size();
            }
            RIt begin(end);
            bool ee = false;
            i32 len = SimpleAppend(url, begin, Tokens.rend(), maxLen, true, ee);
            Tokens.erase(Tokens.begin(), end);
            return len;
        }

        inline It FindBest(It it, It& end) {
            //выбираем слово с наибольшим весом (не учитываем веса слов запроса уже добавленных)
            i64 maxW(-10000);
            It best = end;
            for (; it != end; ++it) {
                if (it->TType != TT_NONE && it->TStatus == TS_NONE) {
                    i64 w = it->GetFreq(Used);
                    if (maxW < w) {
                        maxW = w;
                        best = it;
                    }
                }
            }
            return best;
        }

        struct TCheckResult {
            i32 MiscLen = 0;
            bool HasUsed = false;
            bool NextUsed = false;
        };

        template <class DIt>
        inline TCheckResult Check(DIt& it, DIt end) {
            TCheckResult res;
            DIt base = it;

            //ищем следующее слово и накапливаем длину
            for (; it != end && it->TType == TT_NONE; ++it) {
                res.MiscLen += (i32)it->Len;
            }
            //если нашли слово
            if (it != end) {
                //и оно еще не использован
                if (it->TStatus != TS_USED) {
                    //проверяем есть ли вообще использованые слова дальше
                    ++it;
                    for (; it != end && (it->TType == TT_NONE || it->TStatus != TS_USED); ++it) {
                    }
                    res.HasUsed = (it != end);
                    //ничего кроме самого слова не добавляем
                    it = base;
                    //если использованные слова есть, то рядом с ними уже зарезервированно место под многоточие
                    res.MiscLen = res.HasUsed ? 0 : (i32)Ellipsis.size();
                } else {
                    res.HasUsed = true;
                    res.NextUsed = true;
                    //освобождаем место под многоточие, т.к. соседнее слово использовано уже  (и рядом с ним был резерв)
                    res.MiscLen -= (i32)Ellipsis.size();
                }
            }
            return res;
        }

        inline i32 TryAppend(It& best, It& end, i32 maxLen) {
            size_t len(best->Len);

            //Идет на право - песнь заводи
            It rBorder(best);
            ++rBorder;
            TCheckResult rRes = Check(rBorder, end);
            len += rRes.MiscLen;

            //Налево - сказку говорит
            RIt lit(best);
            TCheckResult lRes = Check(lit, Tokens.rend());
            It lBorder(lit.base());
            len += lRes.MiscLen;

            //если и справа и слева слова используются, то между ними было 1 многоточие
            //а в методе Check мы убрали 2 резерва  (слева и справа) => 1 уборка резерва была лишней
            if (rRes.NextUsed && lRes.NextUsed) {
                len += Ellipsis.size();
            }
            //если с обоих сторон есть использованные слова и они не соседниее
            //то нужен еще 1 многточие
            if (!rRes.NextUsed && rRes.HasUsed && !lRes.NextUsed && lRes.HasUsed) {
                len += Ellipsis.size();
            }
            //если с обоих сторон есть использованные слова, и одно их них не соседнее
            //то нужно вернуть резрв под многоточие, т.к. мы ранее его убрали в Check
            if (lRes.HasUsed && rRes.HasUsed && rRes.NextUsed != lRes.NextUsed) {
                len += Ellipsis.size();
            }
            //если влазит
            if ((i32)len <= maxLen) {
                //добавляем
                for (; lBorder != rBorder; ++lBorder) {
                    lBorder->SetStatus(TS_USED, Used);
                }
                return (i32)len;
            } else {
                //если сейчас не влазит, это не значит что не может влезть вообще (из-за резервов под многоточеи)
                //по этому не выкидывем на совсем сразу, а пессимизируем пока (но быстро, у слова 10 попыток)
                //best->SetStatus(TS_REMOVED, Used);
                if (best->Matches.empty()) {
                    best->AddRevFreq -= 1000;
                } else {
                    best->Matches.clear();
                    best->Gray = 0;
                }
                return 0;
            }
        }

        inline i32 ProcessPart(It& end, i32 maxLen, i32 reserve) {
            if (Tokens.empty())
                return 0;
            It last = end;
            if (last == Tokens.end()) {
                --last; // it's seems ok because Tokens is not empty
            }
            while (Tokens.begin() != last && last->TType == TT_NONE) {
                --last;
            }

            //пока есть что добавляеть, ищем лучшее слово и если влзит, то добавляем
            i32 len(0);
            It best = FindBest(Tokens.begin(), end);
            for (; best != end;
                 best = FindBest(Tokens.begin(), end)) {
                len += TryAppend(best, end, maxLen - len - (best != last && last->TStatus != TS_USED ? 0 : reserve));
            }
            return len;
        }

        inline void Append(THilitedString& url, It& end, i32& maxLen, bool& ee) {
            It it = Tokens.begin();
            for (; it != end; ++it) {
                if (it->TStatus == TS_USED) {
                    url.Append(it->GetHilitedToken());
                    maxLen -= (i32)it->Len;
                    ee = false;
                }
                //если не используется и это первое не используемое слово, то нужно добавить многоточие
                else if (!ee) {
                    url.Append(Ellipsis);
                    maxLen -= (i32)Ellipsis.size();
                    ee = true;
                }
            }
            Tokens.erase(Tokens.begin(), end);
        }

        bool IsEmpty(It& end) {
            bool res = true;
            It it = Tokens.begin();
            for (; res && it != end; res = (it->TType == TT_NONE || !it->Matches.size()), ++it) {
            }
            return res;
        }

        bool IsBadCGI(It& end, i32 maxLen) {
            i32 len(0);
            bool good = false;
            for (It it = Tokens.begin(); it != end; ++it) {
                good |= it->TType != TT_NONE && it->Matches.size() > 0;
                len += (i32)it->Len;
            }
            return ThresholdLen < (MaxLen - maxLen + len) && !good;
        }

        THilitedString GetUrl() {
            i32 maxLen(MaxLen);

            THilitedString url;

            //добавляем домен
            i32 domainLen = ProcessDomain(url, MaxLen);
            const TUtf16String newDomain = DomainTrie.GetDefault(url.String, u"");
            if (!newDomain.empty()) {
                url = THilitedString(newDomain);
                domainLen = newDomain.size();
            }
            maxLen -= domainLen;
            bool ee = false;

            //добавляем путь
            It end = Tokens.begin();
            for (; end != Tokens.end() && end->PType != PT_CGI_SP; ++end) {
            }

            if (IsEmpty(end)) {
                maxLen -= SimpleAppend(url, Tokens.begin(), end, maxLen, false, ee);
                Tokens.erase(Tokens.begin(), end);
            } else {
                size_t tailLen(0);
                if (end != Tokens.end()) {
                    for (It it = end; it != Tokens.end(); ++it) {
                        tailLen += it->Len;
                    }
                    if (tailLen > W_CGI_SEP.size() + Ellipsis.size()) {
                        tailLen = W_CGI_SEP.size() + Ellipsis.size();
                    }
                }
                //если надо, то резервируем место под многоточие
                ProcessPart(end, maxLen, (i32)tailLen);
                Append(url, end, maxLen, ee);
            }

            if (end == Tokens.end()) {
                return url;
            }

            end = Tokens.end();

            if (IsBadCGI(end, maxLen)) {
                if (!ee) {
                    url.Append(Ellipsis);
                }
                return url;
            } else {
                //вырезаем ? из списка токенов
                Tokens.erase(Tokens.begin());

                //добавляем cgi
                i32 cgiLen = ProcessPart(end, i32(maxLen - W_CGI_SEP.size()), 0);

                if (!ee || cgiLen > 0) {
                    url.Append(W_CGI_SEP);
                    ee = false;
                }
                Append(url, end, maxLen, ee);
            }

            return url;
        }
    };

    TUrlCutter::TUrlCutter(TTokenList& tokens, const NSnippets::W2WTrie& domainTrie, i32 maxLen, i32 thresholdLen)
        : Impl(new TUrlCutterImpl(tokens, maxLen, thresholdLen, domainTrie))
    {
    }

    TUrlCutter::~TUrlCutter() {
    }

    THilitedString TUrlCutter::GetUrl() {
        return Impl->GetUrl();
    }
}
