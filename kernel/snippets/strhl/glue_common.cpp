#include "glue_common.h"
#include "zonedstring.h"
#include "zs_transformer.h"
#include "hilite_mark.h"

#include <util/generic/queue.h>
#include <util/generic/singleton.h>
#include <util/charset/wide.h>

namespace {
    struct TIter {
        int Id;
        const TZonedString::TSpans* V;
        size_t I;
        const THiliteMark* Mark;
        TIter(int id, const TZonedString::TZone& z)
          : Id(id)
          , V(&z.Spans)
          , I(0)
          , Mark(z.Mark)
        {
        }
        bool Valid() const {
            return V && I < V->size() && Mark;
        }
        const wchar16* Beg() const {
            return ~(*V)[I];
        }
        const wchar16* End() const {
            return ~(*V)[I] + +(*V)[I];
        }
        bool HasZeroLen() const {
            return !+(*V)[I];
        }
        struct TGtBeg {
            bool operator()(const TIter& a, const TIter& b) const {
                if (a.Beg() != b.Beg()) {
                    return a.Beg() > b.Beg();
                }
                if (a.HasZeroLen() != b.HasZeroLen()) {
                    return b.HasZeroLen();
                }
                if (a.End() != b.End()) {
                    return a.End() < b.End();
                }
                return a.Id > b.Id;
            }
        };
        struct TGtEnd {
            bool operator()(const TIter& a, const TIter& b) const {
                if (a.End() != b.End()) {
                    return a.End() > b.End();
                }
                if (a.HasZeroLen() != b.HasZeroLen()) {
                    return !b.HasZeroLen();
                }
                if (a.Beg() != b.Beg()) {
                    return a.Beg() < b.Beg();
                }
                return a.Id < b.Id;
            }
        };
    };

    struct TEscMap {
        const TUtf16String Quot;
        const TUtf16String Lt;
        const TUtf16String Gt;
        const TUtf16String Ast;
        const TUtf16String Amp;
        TEscMap()
          : Quot(u"&quot;")
          , Lt(u"&lt;")
          , Gt(u"&gt;")
          , Ast(u"&#39;")
          , Amp(u"&amp;")
        {
        }
        const TUtf16String* Get(const wchar16 c) const {
            switch (c) {
                case '\"':
                    return &Quot;
                case '<':
                    return &Lt;
                case '>':
                    return &Gt;
                case '\'':
                    return &Ast;
                case '&':
                    return &Amp;
                default:
                    return nullptr;
            }
        }
    };

    const TUtf16String& GetTag(const TIter& x, const TString& attrWithTag, const TUtf16String& defaultTag) {
        if (attrWithTag.empty())
            return defaultTag;
        const auto& attrs = (*x.V)[x.I].Attrs;
        auto it = attrs.find(attrWithTag);
        return it == attrs.end() ? defaultTag : it->second;
    }
}

namespace NSnippets {
    TUtf16String MergedGlue(const TZonedString& z, const TString& attrWithOpenTag,
        const TString& attrWithCloseTag)
    {
        TUtf16String res;
        TWtringBuf src = z.String;
        TPriorityQueue<TIter, TVector<TIter>, TIter::TGtBeg> qO;
        TPriorityQueue<TIter, TVector<TIter>, TIter::TGtEnd> qC;
        for (TZonedString::TZones::const_iterator it = z.Zones.begin(); it != z.Zones.end(); ++it) {
            TIter i(it->first, it->second);
            if (i.Valid() && i.Beg() < src.data() + src.size()) {
                qO.push(i);
            }
        }
        while (!qO.empty() && qO.top().End() <= src.data()) {
            TIter x = qO.top();
            qO.pop();
            ++x.I;
            if (x.Valid() && x.Beg() < src.data() + src.size()) {
                qO.push(x);
            }
        }
        while (src.size()) {
            size_t d = src.size();
            if (!qO.empty() && qO.top().Beg() < src.data() + d && qO.top().Beg() >= src.data()) {
                d = qO.top().Beg() - src.data();
            }
            if (!qC.empty() && qC.top().End() < src.data() + d && qC.top().End() >= src.data()) {
                d = qC.top().End() - src.data();
            }
            if (d) {
                res.append(src.data(), src.data() + d);
                src.Skip(d);
            }
            while (!qC.empty() && qC.top().End() <= src.data()) {
                TIter x = qC.top();
                qC.pop();
                res += GetTag(x, attrWithCloseTag, x.Mark->CloseTag);
                ++x.I;
                if (x.Valid() && x.Beg() < src.data() + src.size()) {
                    qO.push(x);
                }
            }
            while (!qO.empty() && qO.top().Beg() <= src.data()) {
                TIter x = qO.top();
                qO.pop();
                res += GetTag(x, attrWithOpenTag, x.Mark->OpenTag);
                if (x.Valid()) {
                    qC.push(x);
                }
            }
        }
        while (!qC.empty()) {
            TIter x = qC.top();
            qC.pop();
            res += x.Mark->CloseTag;
        }
        return res;
    }

    TVector<TUtf16String> MergedGlue(const TVector<TZonedString>& z) {
        TVector<TUtf16String> glued;
        for (const TZonedString& zoned : z) {
            glued.push_back(MergedGlue(zoned));
        }
        return glued;
    }

    TZonedString HtmlEscape(const TZonedString& z, bool skipZeroLenSpans) {
        const TEscMap& e = *Singleton<TEscMap>();
        TZonedString res = z;
        TZonedStringTransformer t(res, skipZeroLenSpans);
        for (size_t i = 0; i != res.String.size(); ++i) {
            const TUtf16String* tmp = e.Get(res.String[i]);
            if (tmp) {
                t.Replace(*tmp);
            } else {
                t.Step();
            }
        }
        t.Complete();
        return res;
    }
}
