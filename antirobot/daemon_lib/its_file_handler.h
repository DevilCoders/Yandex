#pragma once

#include "service_param_holder.h"

#include <library/cpp/deprecated/atomic/atomic.h>

#include <util/stream/output.h>
#include <util/folder/path.h>
#include <util/generic/maybe.h>
#include <util/stream/file.h>
#include <util/string/split.h>
#include <util/string/strip.h>

#include <utility>
#include <atomic>

namespace NAntiRobot {
    class IReloadableHandler {
    public:
        virtual void ReloadFile() = 0;
        virtual ~IReloadableHandler() {
        }
    };

    template <typename T>
    class TItsFileHandler: public IReloadableHandler {
    public:
        using TStatusCallback = std::function<void(EHostType, bool)>;

        TItsFileHandler(const TString& handleFilePath, T& data, TMaybe<TStatusCallback> callback = Nothing())
            : Data(data)
            , HandleFilePath(handleFilePath)
            , StatusCallback(std::move(callback))
        {
            ReloadFile();
        }

        void ReloadFile() override {
            TString fileContent = ReadFileContent();

            static_assert(
                IsServiceParamHolder() || IsAtomic() || IsWeightData(),
                "Unknown template type parameter for TItsFileHandler"
            );

            if constexpr (IsServiceParamHolder()) {
                SetByServiceAtomicData(fileContent);
            } else if constexpr (IsAtomic()) {
                SetAtomicData(fileContent);
            } else if constexpr (IsWeightData()) {
                SetWeightData(fileContent);
            }
        }

    private:
        static constexpr bool IsAtomic() {
            return std::is_same_v<T, TAtomic>;
        }

        static constexpr bool IsServiceParamHolder() {
            return std::is_same_v<T, TServiceParamHolder<TAtomic>>;
        }

        static constexpr bool IsWeightData() {
            return std::is_same_v<T, std::atomic<size_t>>;
        }

        TString ReadFileContent() {
            if (!TFsPath(HandleFilePath).Exists()) {
                return {};
            }
            try {
                TFileInput handleStopBlockForAll(HandleFilePath);
                return handleStopBlockForAll.ReadAll();
            } catch (TFileError&) {
                return {};
            }
        }

        void SetByServiceAtomicData(const TString& fileContent) {
            TServiceParamHolder<bool> enabledFlags;

            for (auto serviceName : StringSplitter(fileContent).Split(',')) {
                if (EHostType service; TryFromString(serviceName, service)) {
                    enabledFlags.GetByService(service) = true;
                }
            }

            for (size_t i = 0; i < HOST_NUMTYPES; ++i) {
                auto service = static_cast<EHostType>(i);
                const auto enabled = enabledFlags.GetByService(service);

                if (AtomicGet(Data.GetByService(service)) != enabled) {
                    AtomicSet(Data.GetByService(service), enabled);

                    if (StatusCallback.Defined()) {
                        StatusCallback.GetRef()(service, enabled);
                    }
                }
            }
        }

        void SetAtomicData(const TString& fileData) {
            bool enabled = fileData.Contains("enable");

            if (AtomicGet(Data) != enabled) {
                AtomicSet(Data, enabled);

                if (StatusCallback.Defined()) {
                    auto callback = StatusCallback.GetRef();
                    callback(HOST_NUMTYPES, enabled);
                }
            }
        }

        void SetWeightData(const TString& fileData) {
            const TStringBuf buf = Strip(fileData);
            if (size_t value; !fileData.Empty() && TryFromString(buf, value)) {
                if (value > 100) {
                    value = 100;
                }
                Data = value;
            } else {
                Data = 10;
            }
        }
    private:
        T& Data;
        TString HandleFilePath;
        TMaybe<TStatusCallback> StatusCallback;
    };
}
