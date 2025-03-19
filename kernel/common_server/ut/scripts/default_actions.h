#pragma once

#include "abstract.h"
#include <kernel/common_server/rt_background/manager.h>

namespace NServerTest {

    class TCommonChecker: public ITestAction {
    private:
        using TBase = ITestAction;
        std::function<void(ITestContext& )> Checker;
    protected:
        virtual void DoExecute(ITestContext& context) override;

    public:
        TCommonChecker(const TInstant startInstant, const std::function<void(ITestContext& )>& checker)
            : TBase(startInstant)
              , Checker(checker) {
        }
    };

    class TSleepAction: public ITestAction {
    private:
        using TBase = ITestAction;
    public:
        TSleepAction(const TInstant startInstant)
            : TBase(startInstant) {
        }

    protected:
        virtual void DoExecute(ITestContext& /*context*/) {
            Sleep(GetTimeout());
        }
    };

    class TAPIAction: public ITestAction {
        using TBase = ITestAction;
        using TChecker = std::function< bool(ITestContext& context, const NJson::TJsonValue& json)>;

    public:
        TAPIAction(const TInstant startInstant, const TChecker& checker)
            : TBase(startInstant)
            , Checker(checker) {
        }

    protected:
        TChecker Checker;

        virtual void DoExecute(ITestContext& context);

        virtual TString GetUri(ITestContext& context) const = 0;

        virtual TString GetPostData(ITestContext& context) const {
            auto jsonPost = GetJsonPostData(context);
            return jsonPost.IsNull() ? GetRawPostData(context) : jsonPost.GetStringRobust();
        }

        virtual TString GetRawPostData(ITestContext& /*context*/) const {
            return "";
        }

        virtual NJson::TJsonValue GetJsonPostData(ITestContext& /*context*/) const {
            return NJson::JSON_NULL;
        }

        virtual void CheckReply(ITestContext& context, const NJson::TJsonValue& json) const {
            CHECK_WITH_LOG(Checker(context, json));
        }

        virtual void ProcessReply(ITestContext& context, const NJson::TJsonValue& json) {
            Y_UNUSED(context);
            Y_UNUSED(json);
        }
    };

    class TWaitCommonConditionedActionImpl: public ITestAction {
    private:
        using TBase = ITestAction;
        CS_ACCESS(TWaitCommonConditionedActionImpl, bool, FullWaitingForNegative, false);
    protected:
        virtual void DoExecute(ITestContext& context) override;

        virtual void Init(ITestContext&) const {
        }

        virtual bool Check(ITestContext& context) const = 0;

    public:
        using TBase::TBase;
    };

    class TWaitLambdaAction: public TWaitCommonConditionedActionImpl {
    private:
        using TBase = TWaitCommonConditionedActionImpl;
        std::function<bool(ITestContext&)> Checker;
    protected:
        virtual bool Check(ITestContext& context) const override {
            return Checker(context);
        }

    public:
        TWaitLambdaAction(const TInstant startInstant, const std::function<bool(ITestContext&)>& checker)
            : TBase(startInstant)
            , Checker(checker) {
        }
    };

    template <class T>
    class TRTRobotCreate: public ITestAction {
    private:
        CSA_DEFAULT(TRTRobotCreate, TString, RobotName);
        CSA_FLAG(TRTRobotCreate, Enabled, true);
        CS_ACCESS(TRTRobotCreate, TDuration, Period, TDuration::Seconds(1));
        THolder<T> Settings;
    private:
        using TBase = ITestAction;

    protected:
        virtual void DoExecute(ITestContext& context) override {
            Settings->SetPeriod(Period);
            const TString typeName = Settings->GetType();
            TRTBackgroundProcessContainer process(Settings.Release());
            process.SetEnabled(EnabledFlag);
            process.SetName(RobotName);
            CHECK_WITH_LOG(context.GetServer<IBaseServer>().GetRTBackgroundManager()->GetStorage().ForceUpsertBackgroundSettings(process, "tester"));
        }

    public:
        T* operator->() {
            return Settings.Get();
        }

        TRTRobotCreate(TInstant startInstant, const bool enabled = true)
            : TBase(startInstant)
            , EnabledFlag(enabled)
        {
            Settings.Reset(new T);
            RobotName = Settings->GetType();
        }
    };

    class TRTRobotEnabled: public ITestAction {
    private:
        CSA_DEFAULT(TRTRobotEnabled, TString, RobotName);
        CSA_FLAG(TRTRobotEnabled, Enabled, false);

    private:
        using TBase = ITestAction;

    protected:
        virtual void DoExecute(ITestContext& context) override;

    public:
        TRTRobotEnabled(TInstant startInstant, const TString& robotName, const bool enabled)
            : TBase(startInstant)
            , RobotName(robotName)
            , EnabledFlag(enabled) {
        }
    };

    class TWaitRTRobotExecution: public TWaitCommonConditionedActionImpl {
    private:
        CSA_DEFAULT(TWaitRTRobotExecution, TString, RobotName);
        mutable TSet<TInstant> ExecInstants;
        mutable TMaybe<ui32> BorderSequenceId;
    private:
        using TBase = TWaitCommonConditionedActionImpl;
    public:
        using TBase::TBase;
        TWaitRTRobotExecution(const TInstant startInstant, const TString& robotName)
            : TBase(startInstant)
            , RobotName(robotName) {
        }

    private:
        virtual bool Check(ITestContext& context) const override;
    };

    class TRebuildCaches: public ITestAction {
    private:
        using TBase = ITestAction;
    protected:
        virtual void DoExecute(ITestContext& context) override;
    public:
        using TBase::TBase;
    };

    class TSendRequest: public ITestAction {
    private:
        using TBase = ITestAction;
        CS_ACCESS(TSendRequest, TString, SenderAPI, "self");
        CSA_DEFAULT(TSendRequest, NNeh::THttpRequest, Request);
        TMaybe<ui32> ExpectedCode;
        TMaybe<TString> ExpectedContent;
        TMap<TString, TString> JsonExpectations;
    protected:
        virtual void DoExecute(ITestContext& context) override;
    public:
        using TBase::TBase;
        TSendRequest& ExpectJsonPath(const TString& path, const TString& value) {
            CHECK_WITH_LOG(JsonExpectations.emplace(path, value).second);
            return *this;
        }

        TSendRequest& ExpectCode(const ui32 code) {
            ExpectedCode = code;
            return *this;
        }
        TSendRequest& ExpectContent(const TString& content) {
            ExpectedContent = content;
            return *this;
        }
    };

    class TSetSetting: public ITestAction {
    private:
        CSA_DEFAULT(TSetSetting, TString, Key);
        CSA_DEFAULT(TSetSetting, TString, Value);
        CS_ACCESS(TSetSetting, bool, Waiting, false);
        using TBase = ITestAction;
    protected:
        virtual void DoExecute(ITestContext& context) override;
    public:
        template <class T>
        TSetSetting& SetValue(const T& value) {
            Value = ::ToString(value);
            return *this;
        }
        using TBase::TBase;
    };
}
