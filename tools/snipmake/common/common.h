#pragma once

#include <kernel/qtree/richrequest/richnode.h>
#include <util/generic/string.h>
#include <util/generic/hash.h>

namespace NSnippets {

    struct TSitelink {
        TUtf16String Url;
        TUtf16String Title;
        TUtf16String Snippet;
    };

    struct TSnipFragment {
        TUtf16String Text;
        TString Coords;
        TString ArcCoords;
        TSnipFragment() {
        }
        TSnipFragment(const TUtf16String& text)
          : Text(text)
        {
        }
    };

    struct TSnipMark {
        int Value;
        TUtf16String Criteria;

        TString Label;

        TUtf16String Assessor;
        float Quality;
        TUtf16String Timestamp;

        TSnipMark()
          : Value(0)
          , Quality(0.0f)
        {
        }
    };

    struct TReqSnip {
        TString Query;
        TRichTreePtr RichRequestTree;
        TString B64QTree;
        TUtf16String Url;
        TUtf16String HilitedUrl;
        TString Region;
        TUtf16String TitleText;
        TUtf16String Headline;
        TString HeadlineSrc;
        TVector<TSnipFragment> SnipText;
        TVector<TSitelink> Sitelinks;
        bool IsBNA = false;

        TString Id;
        TString DocumentPath;
        TString FeatureString;
        TString Algo;
        TString Rank;
        TString Lines;
        TString Relevance;
        TVector<TSnipMark> Marks;
        TString ExtraInfo;
        TString WizardType;
        TString ImgUH;

        void MergeFragments();
    };

    struct TReqSerp {
        TString Query;
        TString Region;
        TRichTreePtr RichRequestTree;
        TString B64QTree;
        size_t Id = 0;

        TVector<TReqSnip> Snippets;
        bool HasBNA = false;
    };

    void ParseTsdIntoSnippet(const TString& tsd, TReqSnip* snippet);
    void ParseDumpsnipcandsCandIntoTSnippet(const TString& dumpsnipcandsCand, TReqSnip* snippet);

    class ISnippetsIterator {
    public:
        virtual bool Next() = 0;
        virtual const TReqSnip& Get() const = 0;
        virtual ~ISnippetsIterator() {}
    };

    class ISerpsIterator {
    public:
        virtual bool Next() = 0;
        virtual const TReqSerp& Get() const = 0;
        virtual ~ISerpsIterator() {}
    };
}

Y_DECLARE_OUT_SPEC(inline, NSnippets::TReqSnip, out, r) {
    out << "   id: " << r.Id << Endl
        << "query: " << r.Query << Endl
        << "title: " << r.TitleText << Endl
        << "  url: " << r.Url << Endl
        << "snippet[" << r.SnipText.size() << "]: ";

    for (size_t k=0; k<r.SnipText.size(); ++k) {
        out << r.SnipText[k].Text << "|";
    }
    out << Endl;
}
