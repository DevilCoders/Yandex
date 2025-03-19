#pragma once

#include "abstract.h"

class TFakeReportContext: public IReportBuilderContext {
protected:
    const TCgiParameters& CgiParameters;
    TStringStream Report;
    TInstant RequestBeginTime;

public:
    TFakeReportContext(const TCgiParameters& cgiParameters)
        : CgiParameters(cgiParameters)
        , RequestBeginTime(Now())
    {
    }

    virtual const TCgiParameters& GetCgiParameters() const override {
        return CgiParameters;
    }

    virtual const char* GetUri() const override {
        return Default<TString>().data();
    }

    virtual void MakeSimpleReply(const TBuffer& buf, int /*code*/ = HTTP_OK) override {
        Report << TStringBuf(buf.data(), buf.size());
    }

    const TString& GetReport() const {
        return Report.Str();
    }

    virtual const TSearchRequestData& GetRequestData() const override {
        FAIL_LOG("Not supported");
    }

    virtual TInstant GetRequestStartTime() const override {
        return RequestBeginTime;
    }

    virtual long GetRequestedPage() const override {
        return 0;
    }

    virtual void Print(const TStringBuf& data) override {
        Report << data;
    }

    virtual void AddReplyInfo(const TString& key, const TString& value) override {
        Y_UNUSED(key);
        Y_UNUSED(value);
        FAIL_LOG("Not implemented");
    }
};

class TFakeReportBuilder: public ICustomReportBuilder {
public:
    TFakeReportBuilder()
        : Context(Cgi)
    {
    }

    virtual const IReportBuilderContext& GetContext() const override {
        return Context;
    }
    virtual ui32 GetDocumentsCount() const override {
        return 0;
    }

    virtual void AddDocument(NMetaProtocol::TDocument& /*doc*/) override {
    }
    virtual void AddDocumentToGroup(NMetaProtocol::TDocument& /*doc*/, const TString& /*grouping*/, const TString& /*category*/) override {
    }
    virtual void AddReportProperty(const TString& /*propName*/, const TString& /*propValue*/) override {
    }
    virtual void AddHiddenProperty(const TString& /*propName*/, const TString& /*propValue*/) override {
    }

    virtual void AddErrorMessage(const TString& /*msg*/) override {
    }
    virtual void ConsumeReport(NMetaProtocol::TReport& /*report*/, const TString& /*source*/) override {
    }
    virtual void MarkIncomplete(bool /*value*/ = true) override {
    }
    virtual void ScanReport(IScanner& /*scanner*/) const override {
    }

private:
    TCgiParameters Cgi;
    TFakeReportContext Context;
};
