#include "querylog.h"

#include <contrib/libs/re2/re2/re2.h>

#include <util/string/builder.h>
#include <library/cpp/deprecated/split/split_iterator.h>
#include <util/generic/strbuf.h>

namespace NSnippets {

    TString TQueryLog::ToString() const
    {
        TStringBuilder sb;
        sb << "query=" << Query;
        sb << "\t" << "full-request=" << FullRequest;
        if (!CorrectedQuery.Empty()) {
            sb << "\t" << "msp=" << CorrectedQuery.GetRef();
        }
        sb << "\t" << "user-region=" << UserRegion;
        sb << "\t" << "dom-region=" << DomRegion;
        sb << "\t" << "uil=" << UILanguage;
        sb << "\t" << "reqid=" << RequestId;
        if (!SnipWidth.Empty()) {
            sb << "\t" << "snip-width=" << SnipWidth.GetRef();
        }
        if (!ReportType.Empty()) {
            sb << "\t" << "report=" << ReportType.GetRef();
        }
        for (auto&& doc : Docs) {
            sb << "\t" << "doc=" << doc.Source << ":" << doc.Url;
            sb << "\t" << "snippettype=" << doc.SnippetType;
        }
        if (!MRData.Empty()) {
            sb << "\t\t" << MRData.GetRef();
        }
        return sb;
    }

    TQueryLog TQueryLog::FromString(const TString& str)
    {
        TQueryLog res;
        TSplitDelimiters delims("\t");
        TDelimitersStrictSplit split(str, delims);
        auto it = split.Iterator();
        while (TStringBuf field = it.NextTok()) {
            TStringBuf name = field.NextTok('=');
            TStringBuf value = field;
            if (name == "query") {
                res.Query = value;
            } else if (name == "full-request") {
                res.FullRequest = value;
            } else if (name == "msp") {
                res.CorrectedQuery = TMaybe<TString>(TString(value));
            } else if (name == "user-region") {
                res.UserRegion = value;
            } else if (name == "dom-region") {
                res.DomRegion = value;
            } else if (name == "uil") {
                res.UILanguage = value;
            } else if (name == "reqid") {
                res.RequestId = value;
            } else if (name == "snip-width") {
                res.SnipWidth = TMaybe<TString>(TString(value));
            } else if (name == "report") {
                res.ReportType = TMaybe<TString>(TString(value));
            } else if (name == "doc") {
                TStringBuf source = value.NextTok(':');
                TStringBuf url = value;
                TQueryResultDoc doc;
                doc.Source = source;
                doc.Url = url;
                res.Docs.push_back(doc);
            } else if (name == "snippettype") {
                Y_ASSERT(!res.Docs.empty());
                res.Docs.back().SnippetType = value;
            }
        }
        TStringBuf src(str);
        src.NextTok("\t\t");
        if (!src.empty()) {
            res.MRData = TMaybe<TString>(TString(src));
        }

        return res;
    }

}
