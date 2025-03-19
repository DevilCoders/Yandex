#pragma once
#include <kernel/common_server/library/daemon_base/actions_engine/controller_client.h>

#define PUT_FILE_ACTION_NAME "PUTFILE"

namespace NDaemonController {

    class TPutFileAction : public TAction {
    private:
        TString FileName;
        TString FileData;
    protected:

        virtual NJson::TJsonValue DoSerializeToJson() const override;
        virtual void DoDeserializeFromJson(const NJson::TJsonValue& json) override;

        virtual TString DoBuildCommand() const override {
            return "command=put_file&filename=" + FileName;
        }

        virtual void DoInterpretResult(const TString& result) override;

        static TFactory::TRegistrator<TPutFileAction> Registrator;
    public:

        TPutFileAction() {}

        TPutFileAction(const TString& fileName, const TString& fileData) {
            FileName = fileName;
            FileData = fileData;
        }

        virtual bool GetPostContent(TString& result) const override {
            result = FileData;
            return true;
        }

        virtual TString ActionName() const override { return PUT_FILE_ACTION_NAME; }
    };
}
