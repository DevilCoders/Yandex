#pragma once

#include <kernel/common_server/library/daemon_base/actions_engine/controller_client.h>
#include <library/cpp/string_utils/base64/base64.h>

#define TAKE_FILE_ACTION_NAME "TAKEFILE"

namespace NDaemonController {

    class TTakeFileAction : public TAction {
    private:
        TString FileName;
        TString FromHost;
        ui16 FromPort;
        TString Url;
        TString Post;
        ui64 SleepDurationMs = 100;
    protected:

        virtual NJson::TJsonValue DoSerializeToJson() const override;
        virtual void DoDeserializeFromJson(const NJson::TJsonValue& json) override;

        virtual TString DoBuildCommand() const override {
            return "command=take_file&sleep_duration_ms=" + ToString(SleepDurationMs) + "&filename=" + FileName + "&from_host=" + FromHost + "&from_port=" + ToString(FromPort) + "&url=" + Base64Encode(Url);
        }

        virtual void DoInterpretResult(const TString& result) override;

        static TFactory::TRegistrator<TTakeFileAction> Registrator;
    public:

        TTakeFileAction() {}

        TTakeFileAction(const TString& fileName, const TString& fromHost, ui16 fromPort, const TString& url = Default<TString>(), const TString& post = Default<TString>(), const ui64 sleepDurationMs = 100)
            : FileName(fileName)
            , FromHost(fromHost)
            , FromPort(fromPort)
            , Url(url)
            , Post(post)
            , SleepDurationMs(sleepDurationMs)
        {}

        TTakeFileAction& SetSleepDuration(const ui64 sleepDurationMs) {
            SleepDurationMs = sleepDurationMs;
            return *this;
        }

        virtual ui32 GetAttemptionsMaxResend() const override {
            return 10;
        }

        virtual TDuration GetTimeoutDuration() const override {
            return TDuration::Seconds(20);
        }

        virtual bool GetPostContent(TString& result) const override {
            result = Post;
            return true;
        }

        virtual TString ActionName() const override { return TAKE_FILE_ACTION_NAME; }
    };
}
