#pragma once
#include <kernel/common_server/library/report_builder/abstract.h>
#include <kernel/common_server/library/logging/record/record.h>
#include <kernel/common_server/library/network/data/data.h>

#include <kernel/common_server/obfuscator/obfuscators/abstract.h>

#include <search/output_context/interface.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/logger/global/global.h>

#include <util/datetime/base.h>
#include <util/generic/object_counter.h>
#include <util/memory/blob.h>

enum class EDeadlineCorrectionResult {
    dcrOK /* "not_expired" */,
    dcrNoDeadline /* "no_deadline" */,
    dcrRequestExpired /* "request_expired" */,
    dcrIncorrectDeadline /* "incorrect_deadline" */,
};


class TReportCompressor {
public:
    enum class EType {
        Lz4 /* "lz4" */,
        GZip /* "gzip" */,
    };

    TReportCompressor(const EType& type)
        : Type(type) {}

    TBuffer Process(const TBuffer& report, IReportBuilderContext& context) const;

private:
    EType Type;
};

class TReportEncryptor {
    TString Key;
    ui32 SecretVersion;
public:
    TReportEncryptor(const TString& key, ui32 secretVersion)
        : Key(key)
        , SecretVersion(secretVersion)
    {}

    TBuffer Process(const TBuffer& report, IReportBuilderContext& context) const;
};

class IReplyContext: public IReportBuilderContext, public TObjectCounter<IReplyContext> {
    CSA_DEFAULT(IReplyContext, TString, Link);
    CSA_DEFAULT(IReplyContext, TString, TraceId);
    CS_ACCESS(IReplyContext, TString, ThreadPoolId, "undefined");
private:
    static TAtomic GlobalRequestsCounter;
    TAtomic RequestId;
    TInstant RequestDeadline = TInstant::Max();
    TBlob OverrideBuf;
    THolder<TReportEncryptor> ReportEncryptor;
    THolder<TReportCompressor> ReportCompressor;

protected:
    virtual const TBlob& DoGetBuf() const = 0;
    virtual void DoMakeSimpleReply(const TBuffer& buf, int code = HTTP_OK) = 0;

public:
    using TPtr = TAtomicSharedPtr<IReplyContext>;

public:

    IReplyContext() {
        RequestId = AtomicIncrement(GlobalRequestsCounter);
    }

    void Init(NCS::NLogging::TBaseLogRecord& record, const NCS::NObfuscator::TObfuscatorManagerContainer& obfuscatorManager);

    virtual ~IReplyContext() {}

    virtual void MakeSimpleReply(const TBuffer& buf, int code = HTTP_OK) override final {
        TBuffer compressed;
        if (ReportCompressor) {
            compressed = ReportCompressor->Process(buf, *this);
        }
        if (ReportEncryptor) {
            DoMakeSimpleReply(ReportEncryptor->Process(ReportCompressor ? compressed : buf, *this), code);
        } else {
            DoMakeSimpleReply(ReportCompressor ? compressed : buf, code);
        }
    }

    virtual const TBaseServerRequestData& GetBaseRequestData() const = 0;
    virtual TBaseServerRequestData& MutableBaseRequestData() = 0;

    virtual TCgiParameters& MutableCgiParameters() = 0;
    virtual const TMap<TString, TString>& GetReplyHeaders() const = 0;

    TInstant GetRequestDeadline() const {
        CHECK_WITH_LOG(RequestDeadline != TInstant::Max());
        return RequestDeadline;
    }

    void SetReportEncryptor(THolder<TReportEncryptor> decorator) {
        ReportEncryptor.Reset(decorator.Release());
    }

    void SetReportCompressor(THolder<TReportCompressor> decorator) {
        ReportCompressor.Reset(decorator.Release());
    }

    IReplyContext& SetRequestDeadline(TInstant deadline) {
        RequestDeadline = deadline;
        return *this;
    }

    virtual bool IsHttp() const = 0;
    virtual bool IsLocal() const { return false; }
    virtual NSearch::IOutputContext& Output() = 0;

    virtual ui64 GetRequestId() const final {
        return RequestId;
    }

    void SetBuf(const TString& buf) {
        OverrideBuf = TBlob::FromString(buf);
    }

    virtual const TBlob& GetBuf() const final {
        return OverrideBuf.Empty() ? DoGetBuf() : OverrideBuf;
    }

    virtual TSearchRequestData& MutableRequestData() = 0;

    EDeadlineCorrectionResult DeadlineCorrection(const TDuration& scatterTimeout, const double kffWaitingAvailable);
    EDeadlineCorrectionResult DeadlineCheck(const double kffWaitingAvailable) const;

    virtual long GetRequestedPage() const override {
        return 0;
    }

    virtual void Print(const TStringBuf& data) override {
        Output().Write(data);
    }
};

template <class TBase>
class IRDReplyContext: public TBase {
protected:
    virtual TStringBuf DoGetUri() const override {
        return GetRequestData().ScriptName();
    }
public:
    using TBase::TBase;
    using TBase::GetRequestData;
    using TBase::MutableRequestData;

    virtual const TCgiParameters& GetCgiParameters() const override {
        return GetRequestData().CgiParam;
    }

    virtual const TBaseServerRequestData& GetBaseRequestData() const override {
        return GetRequestData();
    }

    virtual TBaseServerRequestData& MutableBaseRequestData() override {
        return MutableRequestData();
    }

    virtual TCgiParameters& MutableCgiParameters() override {
        return MutableRequestData().CgiParam;
    }
};
