#pragma once

#include <kernel/reqbundle/reqbundle.h>
#include <kernel/reqbundle/request_splitter.h>

class TReqBundleBuilder {
public:
    struct TSynonym {
        TStringBuf Text;
        size_t FirstWord;
        size_t LastWord;
    };

public:
    using TOptions = NReqBundle::TRequestSplitter::TOptions;

    TReqBundleBuilder(const TOptions& options = TOptions())
        : Bundle(new NReqBundle::TReqBundle)
        , Splitter(*Bundle, options)
    {}

    TReqBundleBuilder& operator << (TStringBuf text) {
        if (!CurText.empty()) {
            Flush();
        }
        CurText = text;
        HasRequest = true;
        return *this;
    }
    TReqBundleBuilder& operator << (const TSynonym& synonym) {
        Y_VERIFY(HasRequest);
        CurSyn.push_back(synonym);
        return *this;
    }
    TReqBundleBuilder& operator << (const NReqBundle::TFacetId& facet) {
        Y_VERIFY(HasRequest);
        CurFacets.push_back(std::make_pair(facet, 1.0f));
        return *this;
    }
    TReqBundleBuilder& operator << (float value) {
        Y_VERIFY(HasRequest);
        Y_VERIFY(!CurFacets.empty());
        CurFacets.back().second = value;
        return *this;
    }

    NReqBundle::TReqBundlePtr GetResult() {
        if (HasRequest) {
            Flush();
        }
        return Bundle;
    }

private:
    void Flush() {
        Y_VERIFY(HasRequest);
        auto reqPtr = Splitter.SplitRequest(CurText, TLangMask(), CurFacets);

        if (reqPtr) {
            for (auto& syn : CurSyn) {
                Splitter.AddSynonym(*reqPtr, syn.Text, syn.FirstWord, syn.LastWord);
            }
        } else {
            Y_VERIFY(CurSyn.empty());
        }

        CurText = "";
        HasRequest = false;
        CurFacets.clear();
        CurSyn.clear();
    }

private:
    NReqBundle::TReqBundlePtr Bundle;
    NReqBundle::TRequestSplitter Splitter;

    bool HasRequest = false;
    TStringBuf CurText;
    TVector<std::pair<NReqBundle::TFacetId, float>> CurFacets;
    TVector<TSynonym> CurSyn;
};

inline TReqBundleBuilder::TSynonym MakeSynonym(TStringBuf text, size_t fromIndex, size_t toIndex) {
    return TReqBundleBuilder::TSynonym{text, fromIndex, toIndex};
}
