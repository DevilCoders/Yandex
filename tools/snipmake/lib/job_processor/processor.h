#pragma once

#include <tools/snipmake/metasnip/jobqueue/mtptsr.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/string/cast.h>

namespace NSnippets {

struct IContextData {
    virtual const TString& GetRawInput() const = 0;
    virtual ~IContextData() {
    }
};

template<typename TContextData>
struct ITInputProcessor {
    virtual bool Next() = 0;
    virtual TContextData& GetContextData() = 0;
    virtual TContextData* GetExpContextData() {
        return nullptr;
    }
    virtual ~ITInputProcessor() {
    }
};

template<typename TContextData>
struct ITConverter {
    virtual void Convert(const TContextData& contextData) = 0;
    virtual ~ITConverter() {
    }
};

template<typename TContextData>
struct TTPairedInput : public ITInputProcessor<TContextData> {
    ITInputProcessor<TContextData>& Base;
    THolder<ITInputProcessor<TContextData>> Exp;

    TTPairedInput(ITInputProcessor<TContextData>& base, ITInputProcessor<TContextData>* exp)
        : Base(base)
        , Exp(exp)
    {
        Y_ASSERT(exp);
    }
    bool Next() override {
        return Base.Next() && Exp->Next();
    }
    TContextData& GetContextData() override {
        return Base.GetContextData();
    }
    TContextData* GetExpContextData() override {
        return &Exp->GetContextData();
    }
};

class TThreadData {
private:
    class TImpl;

    THolder<TImpl> Impl;
public:
    TThreadData(bool log, const TString& name);
    ~TThreadData();
    void Log(const IContextData& ctxData);
};

struct TThreadDataManager : NSnippets::IThreadSpecificResourceManager {
    bool DeadLog = false;
    TString LogPrefix;
    size_t LogIdx = 1;
    TThreadDataManager(bool deadLog, const TString& logPrefix = TString())
      : DeadLog(deadLog)
      , LogPrefix(logPrefix)
    {
    }
    void* CreateThreadSpecificResource() override {
        return new TThreadData(DeadLog, LogPrefix + ToString(LogIdx++));
    }
    void DestroyThreadSpecificResource(void* tsr) override {
        delete ((TThreadData*)tsr);
    }
};

template<typename TJob>
struct ITOutputProcessor {
    virtual void Process(const TJob& job) = 0;
    virtual void Complete() {
    }
    virtual ~ITOutputProcessor() {
    }
};
};
