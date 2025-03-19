#pragma once

#include "abstract.h"

#include <search/idl/meta.pb.h>

#include <util/generic/vector.h>

class TSelfFlushLogFrame;
using TSelfFlushLogFramePtr = TIntrusivePtr<TSelfFlushLogFrame>;

class TRTYSimpleProtoReportBuilder: public ICustomReportBuilder {
private:
    using TGroup     = TVector<NMetaProtocol::TDocument>;
    using TGrouping  = TMap<TString, TGroup>;
    using TGroupings = TMap<TString, TGrouping>;

private:
    IReportBuilderContext& Context;

    NMetaProtocol::TReport Report;
    NMetaProtocol::TSearchProperties Hidden;
    TGroupings Groupings;
    TString Errors;
    TVector<TString> Events;
    ui64 Code;
    bool Complete;

public:
    TRTYSimpleProtoReportBuilder(IReportBuilderContext& context)
        : Context(context)
        , Code(HTTP_OK)
        , Complete(true)
    {
    }

    virtual const IReportBuilderContext& GetContext() const override;
    virtual ui64 GetCode() const override;
    virtual ui32 GetDocumentsCount() const override;
    virtual void AddDocument(NMetaProtocol::TDocument& doc) override;
    virtual void AddDocumentToGroup(NMetaProtocol::TDocument& doc, const TString& grouping, const TString& category) override;
    virtual void AddReportProperty(const TString& propName, const TString& propValue) override;
    virtual void AddHiddenProperty(const TString& propName, const TString& propValue) override;
    virtual void AddErrorMessage(const TString& msg) override;
    virtual void ConsumeReport(NMetaProtocol::TReport& report, const TString& source) override;
    virtual void MarkIncomplete(bool value = true) override;
    virtual void ScanReport(IScanner& scanner) const override;

    void ConsumeLog(const TSelfFlushLogFramePtr eventLog);
    void Finish(const ui32 httpCode);
    void Finish(NMetaProtocol::TReport&& report, const ui32 httpCode);

protected:
    inline IReportBuilderContext& GetContext() {
        return Context;
    }

    void InsertDocuments(NMetaProtocol::TReport& report);
    void InsertErrors(NMetaProtocol::TReport& report) const;
    void InsertEventLog(NMetaProtocol::TReport& report) const;
    void InsertMeta(NMetaProtocol::TReport& report, const ui32 httpCode) const;

    virtual void FinishImpl(NMetaProtocol::TReport& report, const ui32 httpCode);

private:
    ui32 GetDocumentsCount(const TGroupings& groupings) const;
    ui32 GetDocumentsCount(const TGrouping& grouping) const;
    ui32 GetDocumentsCount(const TGroup& group) const;
};
