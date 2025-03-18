#include "event.h"
#include "parstypes.h"

#include <util/generic/map.h>
#include <util/generic/singleton.h>
#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/stream/output.h>
#include <util/stream/str.h>

namespace {
    struct TRep: public TMap<char, TString> {
        inline TRep() {
            (*this)['\0'] = "\\0";
            (*this)['\t'] = "\\t";
            (*this)['\r'] = "\\r";
            (*this)['\n'] = "\\n";
            (*this)['\f'] = "\\f";
            (*this)['\v'] = "\\v";
            (*this)['\\'] = "\\\\";
            (*this)['\''] = "\\'";
            (*this)['\"'] = "\\\"";
        }

        static inline const TRep& Instance() {
            return *Singleton<TRep>();
        }
    };

    TString nicer(const TString& s) {
        const TRep& rep = TRep::Instance();
        TStringStream os;

        for (size_t i = 0; i < s.size(); ++i) {
            TRep::const_iterator r = rep.find(s[i]);

            if (r != rep.end())
                os << r->second;
            else
                os << s[i];
        }

        return os.Str();
    }
}

template <>
void Out<HT_TAG>(IOutputStream& os, TTypeTraits<HT_TAG>::TFuncParam tag) {
    os << "TAG: " << (size_t)tag << ' ' << NHtml::FindTag(tag).name;
}

template <>
void Out<THtmlChunk>(IOutputStream& os, TTypeTraits<THtmlChunk>::TFuncParam e) {
    os << "{"
       << "{ "
       << BREAK_TYPE(e.flags.brk) << ", "
       << SPACE_MODE(e.flags.space) << ", "
       << ATTR_TYPE(e.flags.atype) << ", "

       // if(e.flags.type == PARSED_ATTRIBUTE)
       //      apos is no longer in use

       << HTLEX_TYPE(e.flags.apos) << ", "
       << TEXT_WEIGHT(e.flags.weight) << ", "
       << MARKUP_TYPE(e.flags.markup) << ", "
       << PARSED_TYPE(e.flags.type)
       << "}, "
       << "NULL, " // this line is only needed to match test output
       << '"' << nicer(TString(e.text, e.leng)) << '"' << ", "
       << e.leng
       << '}';
}
