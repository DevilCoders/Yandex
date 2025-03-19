#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

#include <functional>

namespace NQueryData {
    class TSourceFactors;
    class TQueryData;

    struct TDocView {
        TStringBuf Url;
        TStringBuf DocId;
        TStringBuf Owner;

        TDocView& SetUrl(TStringBuf url) {
            Url = url;
            return *this;
        }

        TDocView& SetDocId(TStringBuf docId) {
            DocId = docId;
            return *this;
        }

        TDocView& SetOwner(TStringBuf owner) {
            Owner = owner;
            return *this;
        }
    };


    bool ResponseMatchesDocument(const TSourceFactors&, const TDocView& d);


    class IDocumentResponseProcessor {
    public:
        virtual ~IDocumentResponseProcessor() {}

        virtual void OnUrl(TStringBuf /*url*/, const TSourceFactors&) {}
        virtual void OnDocId(TStringBuf /*docId*/, const TSourceFactors&) {}
        virtual void OnOwner(TStringBuf /*owner*/, const TSourceFactors&) {}
    };


    class TUrl2DocIdResponseProcessor : public IDocumentResponseProcessor {
        TString Url2DocIdBuf;

    public:
        void OnUrl(TStringBuf url, const TSourceFactors&) override;
    };


    void ProcessDocumentResponses(IDocumentResponseProcessor&, const TQueryData&, bool needSorting = false);


    using TOnItemResponse = std::function<void(TStringBuf, const NQueryData::TSourceFactors&)>;

    // IDocumentResponseProcessor::OnUrl
    void ProcessUrlResponses(TOnItemResponse, const TQueryData&, bool needSorting = false);
    void ProcessUrlResponses(TOnItemResponse, const TSourceFactors&);

    // IDocumentResponseProcessor::OnDocId + IDocumentResponseProcessor::OnUrl
    void ProcessDocIdResponses(TOnItemResponse, const TQueryData&, bool needSorting = false);
    void ProcessDocIdResponses(TOnItemResponse, const TSourceFactors&);

    // IDocumentResponseProcessor::OnOwner
    void ProcessOwnerResponses(TOnItemResponse, const TQueryData&, bool needSorting = false);
    void ProcessOwnerResponses(TOnItemResponse, const TSourceFactors&);

}
