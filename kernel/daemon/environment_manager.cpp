#include "environment_manager.h"

#include <kernel/daemon/common/time_guard.h>

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/svnversion/svnversion.h>

#include <google/protobuf/text_format.h>

#include <util/stream/file.h>

namespace {
    class TControllerStatusGuard {
    private:
        TString FileName;
        bool ReadOnly;
        NController::TControllerStatus Status;
        static TMutex Mutex;

    public:
        const NController::TControllerStatus& GetStatus() const {
            return Status;
        }

        NController::TControllerStatus& MutableStatus() {
            VERIFY_WITH_LOG(!ReadOnly, "invalid usage of TControllerStatusGuard");
            return Status;
        }

        TControllerStatusGuard(const TString& fileName, bool readOnly = false)
            : FileName(fileName)
            , ReadOnly(readOnly)
        {
            TGuard<TMutex> g(Mutex);
            try {
                CHECK_WITH_LOG(!!FileName);
                if (TFsPath(fileName).Exists()) {
                    TUnbufferedFileInput fi(fileName);
                    TString text = fi.ReadAll();
                    CHECK_WITH_LOG(::google::protobuf::TextFormat::ParseFromString(text, &Status));
                } else {
                    Status.SetStatus(NController::OK);
                }
            } catch (...) {
                ERROR_LOG << "Can't read status file(" << fileName << "): " << CurrentExceptionMessage() << Endl;
            }
        }

        ~TControllerStatusGuard() {
            if (!FileName || ReadOnly)
                return;
            TGuard<TMutex> g(Mutex);
            try {
                TString result;
                CHECK_WITH_LOG(::google::protobuf::TextFormat::PrintToString(Status, &result));
                TUnbufferedFileOutput fo(FileName);
                fo << result;
                NOTICE_LOG << "write status file(" << FileName << "): " << result << Endl;
            } catch (...) {
                ERROR_LOG << "Can't write status file(" << FileName << "): " << CurrentExceptionMessage() << Endl;
            }
        }

        void Release() {
            FileName.clear();
        }
    };

    TMutex TControllerStatusGuard::Mutex;
}

namespace NController {

    NJson::TJsonValue TEnvironmentManager::BuildReport() const {
        NJson::TJsonValue result(NJson::JSON_MAP);
        if (!CurrentId) {
            return result;
        }
        TControllerStatusGuard csgLocal(ConfigurationsPath / CurrentId / "status", true);
        TControllerStatusGuard csg(ConfigurationsPath / "GLOBAL" / "status", true);
        if (csgLocal.GetStatus().GetStatus() == NController::OK && csg.GetStatus().GetRestore()) {
            result["state"] = NController::TSlotStatus_Name(NController::RestoredConfig);
            result["info"] = "Configs restored";
        } else {
            result["state"] = NController::TSlotStatus_Name(csgLocal.GetStatus().GetStatus());
            result["info"] = csgLocal.GetStatus().GetInfo();
        }
        result["restore"] = csg.GetStatus().GetRestore();
        return result;
    }

    class TConfigurationId {
    private:
        TString Id;
    public:
        bool GetRevision(int& result) const {
            size_t idx = Id.find('-');
            if (idx == TString::npos) {
                return false;
            } else {
                return TryFromString<int>(Id.substr(idx + 1), result);
            }
        }

        TConfigurationId(const TString& id)
            : Id(id)
        {
        }

        TConfigurationId(const TFsPath& configs) {
            MD5 md5Calcer;
            TVector<TFsPath> listFilesV = { configs };
            TSet<TString> listFiles;
            for (ui32 i = 0; i < listFilesV.size(); ++i) {
                TFsPath path = listFilesV[i];
                if (path.IsDirectory()) {
                    path.List(listFilesV);
                } else {
                    listFiles.insert(path.GetPath());
                }
            }
            for (auto&& i : listFiles) {
                TUnbufferedFileInput fi(i);
                md5Calcer.Update(fi.ReadAll());
            }
            char buf[33] = { 0 };
            md5Calcer.End(buf);
            const int revision = GetProgramSvnRevision();

            Id = TString(buf) + "-" + ToString(revision);
        }

        const TString& Get() const {
            return Id;
        }
    };

    TEnvironmentManager::TEnvironmentManager(TServerConfigConstructorParams& params)
        : Params(params)
    {
        auto daemonConfig = Params.GetDaemonConfig();
        RootPath = daemonConfig->GetController().StateRoot + "/" + ToString(daemonConfig->GetController().Port);
        ConfigurationsPath = RootPath / "configurations";
        if (ConfigurationsPath.Exists()) {
            TVector<TFsPath> configurations;
            ConfigurationsPath.List(configurations);
            TMap<ui32, TVector<TFsPath>> pathesByRevision;
            ui32 counterConfigurations = 0;
            for (auto&& i : configurations) {
                TConfigurationId id(i.GetName());
                int revision;
                if (id.GetRevision(revision)) {
                    pathesByRevision[revision].emplace_back(i);
                    ++counterConfigurations;
                }
            }
            INFO_LOG << "Stored configurations count: " << counterConfigurations << Endl;
            ui32 counter = 0;
            for (auto&& i = pathesByRevision.rbegin(); i != pathesByRevision.rend(); ++i) {
                if (counter < 10) {
                    counter += i->second.size();
                    continue;
                }
                for (auto&& d : i->second) {
                    INFO_LOG << "Clean configuration: " << d.GetPath() << Endl;
                    d.ForceDelete();
                }
            }
        }
        TFsPath(ConfigurationsPath / "GLOBAL").MkDirs();
    }

    const TString& TEnvironmentManager::GetCurrentId() const {
        CHECK_WITH_LOG(!!CurrentId);
        return CurrentId;
    }

    TString TEnvironmentManager::GetLastSuccessConfiguration() const {
        TControllerStatusGuard csg(ConfigurationsPath / "GLOBAL" / "status", true);
        return csg.GetStatus().GetSuccessConfiguration();
    }

    void TEnvironmentManager::Clear() {
        CHECK_WITH_LOG(!!CurrentId);
        SetStatus("", NController::OK);
        ClearGlobal();
    }

    void TEnvironmentManager::ClearGlobal() {
        TControllerStatusGuard csg(ConfigurationsPath / "GLOBAL" / "status", false);
        csg.MutableStatus().SetRestore(false);
    }

    void TEnvironmentManager::ClearConfigurations() {
        if (ConfigurationsPath.Exists()) {
            TWriteGuard guard(Mutex);
            auto globalConfigurationPath = ConfigurationsPath / "GLOBAL";
            ConfigurationsPath.ForceDelete();
            globalConfigurationPath.MkDirs();
        }
    }

    void TEnvironmentManager::Success() {
        CHECK_WITH_LOG(!!CurrentId);
        SetStatus("", NController::OK);
        TControllerStatusGuard csg(ConfigurationsPath / "GLOBAL" / "status", false);
        csg.MutableStatus().SetSuccessConfiguration(CurrentId);
        csg.MutableStatus().SetRestartAttemption(0);
    }

    NController::TSlotStatus TEnvironmentManager::GetStatus(const TString& id /*= ""*/) const {
        TString idReal = !!id ? id : CurrentId;
        CHECK_WITH_LOG(!!idReal);
        TControllerStatusGuard csg(ConfigurationsPath / idReal / "status", true);
        return csg.GetStatus().GetStatus();
    }

    ui32 TEnvironmentManager::IncrementRestartAttemptions(const TString& id) const {
        TString idReal = !!id ? id : CurrentId;
        CHECK_WITH_LOG(!!idReal);
        TControllerStatusGuard csg(ConfigurationsPath / idReal / "status", false);
        const ui32 attemptions = csg.MutableStatus().GetRestartAttemption() + 1;
        csg.MutableStatus().SetRestartAttemption(attemptions);
        return attemptions;
    }

    void TEnvironmentManager::ResetRestartAttemptions(const TString& id) const {
        TString idReal = !!id ? id : CurrentId;
        CHECK_WITH_LOG(!!idReal);
        TControllerStatusGuard csg(ConfigurationsPath / idReal / "status", false);
        csg.MutableStatus().SetRestartAttemption(0);
    }

    bool TEnvironmentManager::IsRestored() const {
        TControllerStatusGuard csg(ConfigurationsPath / "GLOBAL" / "status", true);
        return csg.GetStatus().GetRestore();
    }

    void TEnvironmentManager::SetStatus(const TString& info, const TSystemStatusMessage::ESystemStatus status, const TString& id /*= ""*/) const {
        TString idReal = !!id ? id : CurrentId;
        CHECK_WITH_LOG(!!idReal);
        TControllerStatusGuard csg(ConfigurationsPath / idReal / "status");

        NController::TSlotStatus statusNew = NController::TSlotStatus::UnknownError;
        if (status == TSystemStatusMessage::ESystemStatus::ssIncorrectConfig) {
            statusNew = NController::TSlotStatus::FailedConfig;
        } else if (status == TSystemStatusMessage::ESystemStatus::ssIncorrectData) {
            statusNew = NController::TSlotStatus::FailedIndex;
        } else if (status == TSystemStatusMessage::ESystemStatus::ssOK) {
            statusNew = NController::TSlotStatus::OK;
        } else if (status == TSystemStatusMessage::ESystemStatus::ssUnknownError) {
            statusNew = NController::TSlotStatus::UnknownError;
        } else {
            CHECK_WITH_LOG("Incorrect status");
        }

        if (statusNew != csg.GetStatus().GetStatus() || info != csg.GetStatus().GetInfo()) {
            FATAL_LOG << "InfoServerSignal : " << info << "/" << NController::TSlotStatus_Name(statusNew) << Endl;
            csg.MutableStatus().SetStatus(statusNew);
            csg.MutableStatus().SetInfo(info);
        } else {
            csg.Release();
        }
    }

    void TEnvironmentManager::SetStatusImpl(const TString* message, const NController::TSlotStatus& status, const TString& id /*= ""*/) const {
        TString idReal = !!id ? id : CurrentId;
        CHECK_WITH_LOG(!!idReal);
        TControllerStatusGuard csg(ConfigurationsPath / idReal / "status", false);
        if (status != csg.GetStatus().GetStatus() || (message && *message != csg.GetStatus().GetInfo())) {
            csg.MutableStatus().SetStatus(status);
            if (message) {
                csg.MutableStatus().SetInfo(*message);
            }
        } else {
            csg.Release();
        }
    }

    void TEnvironmentManager::SetStatus(const TString& message, const NController::TSlotStatus& status, const TString& id /*= ""*/) const {
        SetStatusImpl(&message, status, id);
    }

    void TEnvironmentManager::SetStatus(const NController::TSlotStatus& status, const TString& id /*= ""*/) const {
        SetStatusImpl(nullptr, status, id);
    }

    bool TEnvironmentManager::CheckCurrent() {
        TTimeGuardImpl<false, ELogPriority::TLOG_NOTICE> tg("TEnvironmentManager::CheckCurrent");
        TWriteGuard wg(Mutex);
        auto daemonConfig = Params.GetDaemonConfig();
        TFsPath from(daemonConfig->GetController().ConfigsRoot);

        TConfigurationId id(from);
        CurrentId = id.Get();
        TFsPath path(ConfigurationsPath / CurrentId);
        if (path.Exists()) {
            return true;
        }

        TFsPath pathStorage(path / "configs");
        from.CopyTo(pathStorage, true);

        return false;
    }

    bool TEnvironmentManager::RestoreConfiguration(const TString& configurationId) const {
        INFO_LOG << "Configuration restoring: " << configurationId << Endl;
        auto daemonConfig = Params.GetDaemonConfig();
        TFsPath path(ConfigurationsPath / configurationId / "configs");
        if (path.Exists()) {
            TFsPath to(daemonConfig->GetController().ConfigsRoot);
            to.ForceDelete();
            path.CopyTo(to, true);
            TControllerStatusGuard csg(ConfigurationsPath / "GLOBAL" / "status", false);
            csg.MutableStatus().SetRestore(true);
            return true;
        } else {
            NOTICE_LOG << "Did not find correct configs in " << path.GetPath() << Endl;
            return false;
        }
    }
}
