#pragma once
#include <kernel/common_server/library/daemon_base/actions_engine/controller_client.h>

#define DELETE_FILE_ACTION_NAME "DELFILE"

namespace NDaemonController {

    class TDeleteFileAction : public TAction {
    private:
        TString FileName;
    protected:

        virtual NJson::TJsonValue DoSerializeToJson() const override;
        virtual void DoDeserializeFromJson(const NJson::TJsonValue& json) override;

        virtual TString DoBuildCommand() const override {
            return "command=delete_file&filename=" + FileName;
        }

        virtual void DoInterpretResult(const TString& result) override;

        static TFactory::TRegistrator<TDeleteFileAction> Registrator;
    public:

        TDeleteFileAction() {}

        TDeleteFileAction(const TString& fileName) {
            FileName = fileName;
        }

        virtual TString ActionName() const override { return DELETE_FILE_ACTION_NAME; }
    };
}
