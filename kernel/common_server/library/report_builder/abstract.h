#pragma once

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/http/misc/httpreqdata.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/generic/buffer.h>

enum ERTYSearchResult {
    SR_OK,
    SR_NOT_FOUND
};

class TSearchRequestData;

namespace NMetaProtocol {
    class TDocument;
    class TReport;
}

class IReportBuilderContext {
protected:
    virtual TStringBuf DoGetUri() const = 0;
public:
    virtual ~IReportBuilderContext() {}
    virtual TString GetUri(const TString& defaultUri = "") const;
    virtual const TCgiParameters& GetCgiParameters() const = 0;
    virtual const TSearchRequestData& GetRequestData() const = 0;
    virtual TInstant GetRequestStartTime() const = 0;
    virtual long GetRequestedPage() const = 0;

    virtual void MakeSimpleReply(const TBuffer& buf, int code = HTTP_OK) = 0;
    virtual void AddReplyInfo(const TString& key, const TString& value, const bool rewrite = true) = 0;
    virtual void Print(const TStringBuf& data) = 0;
};

class ICustomReportBuilder {
public:
    class IScanner {
    public:
        virtual ~IScanner() {}

        virtual void operator()(const NMetaProtocol::TDocument& doc, const TString& grouping, const TString& category) = 0;
        virtual void operator()(const TString& key, const TString& value) = 0;
    };

public:
    virtual ~ICustomReportBuilder() {}

    virtual const IReportBuilderContext& GetContext() const = 0;
    virtual ui64 GetCode() const { return 0; }
    virtual ui32 GetDocumentsCount() const = 0;

    virtual void AddDocument(NMetaProtocol::TDocument& doc) = 0;
    virtual void AddDocumentToGroup(NMetaProtocol::TDocument& doc, const TString& grouping, const TString& category) = 0;
    virtual void AddReportProperty(const TString& propName, const TString& propValue) = 0;
    virtual void AddHiddenProperty(const TString& propName, const TString& propValue) = 0;

    template <class T>
    void AddReportProperty(const TString& propName, const T& propValue) {
        AddReportProperty(propName, ToString(propValue));
    }

    virtual void AddErrorMessage(const TString& msg) = 0;
    virtual void ConsumeReport(NMetaProtocol::TReport& report, const TString& source) = 0;
    virtual void MarkIncomplete(bool value = true) = 0;
    virtual void ScanReport(IScanner& scanner) const = 0;
};
