#pragma once

#include <kernel/daemon/config/config_constructor.h>
#include <kernel/daemon/protos/status.pb.h>

#include <library/cpp/mediator/global_notifications/system_status.h>

#include <util/folder/path.h>

namespace NController {

    class TEnvironmentManager {
    private:
        TServerConfigConstructorParams& Params;
        TString CurrentId;
        TRWMutex Mutex;
        TFsPath ConfigurationsPath;
        TFsPath RootPath;

    private:
        void SetStatusImpl(const TString* message, const NController::TSlotStatus& status, const TString& id = "") const;

    public:
        TEnvironmentManager(TServerConfigConstructorParams& params);

        void Clear();
        void ClearGlobal();
        void ClearConfigurations();
        bool CheckCurrent();
        bool IsRestored() const;
        bool RestoreConfiguration(const TString& configurationId) const;
        void Success();

        NJson::TJsonValue BuildReport() const;
        const TString& GetCurrentId() const;
        TString GetLastSuccessConfiguration() const;
        NController::TSlotStatus GetStatus(const TString& id = "") const;
        ui32 IncrementRestartAttemptions(const TString& id) const;
        void ResetRestartAttemptions(const TString& id) const;

        void SetStatus(const TString& message, const NController::TSlotStatus& status, const TString& id = "") const;
        void SetStatus(const NController::TSlotStatus& status, const TString& id = "") const;
        void SetStatus(const TString& info, const TSystemStatusMessage::ESystemStatus status, const TString& id = "") const;
    };

    class TStatusGuard {
    private:
        NController::TSlotStatus StatusStored;
        TEnvironmentManager* Manager;

    public:
        TStatusGuard(TEnvironmentManager* manager, const NController::TSlotStatus activeStatus)
            : Manager(manager)
        {
            if (Manager) {
                StatusStored = Manager->GetStatus();
                Manager->SetStatus(activeStatus);
            }
        }

        ~TStatusGuard() {
            if (Manager) {
                Manager->SetStatus(StatusStored);
            }
        }
    };
}
