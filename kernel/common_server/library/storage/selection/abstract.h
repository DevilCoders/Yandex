#pragma once

#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/library/storage/query/request.h>
#include <kernel/common_server/util/cgi_processing.h>
#include <kernel/common_server/library/scheme/scheme.h>

namespace NCS {
    namespace NSelection {
        class IExternalData {
        public:
            using TPtr = TAtomicSharedPtr<IExternalData>;
            virtual ~IExternalData() = default;
        };

        class IFilter {
        public:
            using TPtr = TAtomicSharedPtr<IFilter>;
            virtual ~IFilter() = default;

            virtual void FillFilter(TSRMulti& srMulti, IExternalData::TPtr externalData) const = 0;
            virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;
            virtual void FillScheme(NCS::NScheme::TScheme& scheme) const = 0;
        };

        class IReader {
        public:
            virtual ~IReader() = default;
            virtual bool Read(NStorage::ITransaction& tr, IExternalData::TPtr externalData = nullptr) = 0;
            virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;
            virtual bool GetHasMore() const = 0;
            virtual void SetCountLimit(const ui32 value) = 0;
            virtual void DropCountLimit() = 0;
            virtual bool GetNoDataOnEmptyFilter() const = 0;
            virtual void SetNoDataOnEmptyFilter(const bool value) = 0;
            virtual TString SerializeCursorToString() const = 0;
            virtual TMaybe<NJson::TJsonValue> SerializeAdditionalInfoToJson() const {
                return Nothing();
            }
        };

        template <class TObject>
        class IObjectsReader: public IReader {
        protected:
            virtual TVector<TObject> DoDetachObjects() = 0;
        public:
            using TPtr = TAtomicSharedPtr<IObjectsReader<TObject>>;
            TVector<TObject> DetachObjects() {
                return std::move(DoDetachObjects());
            }
        };

        template <class TObject>
        class IBaseObjectsReader: public IObjectsReader<TObject> {
        protected:
            bool HasMore = true;
            TMaybe<ui32> CountLimit;
            bool NoDataOnEmptyFilter = true;
            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;

        public:
            virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override final {
                if (!TJsonProcessor::Read(jsonInfo, { "count_limit", "limit" }, CountLimit)) {
                    return false;
                }
                if (!CountLimit && !NoDataOnEmptyFilter) {
                    CountLimit = 10;
                }
                return DoDeserializeFromJson(jsonInfo);
            }

            virtual bool GetHasMore() const override final {
                return HasMore;
            }
            virtual void SetCountLimit(const ui32 value) override final {
                CountLimit = value;
            }
            virtual void DropCountLimit() override final {
                CountLimit.Clear();
            }
            virtual bool GetNoDataOnEmptyFilter() const override final {
                return NoDataOnEmptyFilter;
            }
            virtual void SetNoDataOnEmptyFilter(const bool value) override final {
                NoDataOnEmptyFilter = value;
            }
        };

        template <class TResultObject, class TOriginalObject>
        class IAdaptiveObjectsReader: public IObjectsReader<TResultObject> {
        private:
            using TBase = IObjectsReader<TResultObject>;
            typename IObjectsReader<TOriginalObject>::TPtr OriginalReader;
        protected:
            virtual bool DoAdaptObjects(TVector<TOriginalObject>&& originalObjects, TVector<TResultObject>& result) const = 0;

            virtual bool AdaptObjects(TVector<TOriginalObject>&& originalObjects, TVector<TResultObject>& result) const {
                auto gLogging = TFLRecords::StartContext().Method("AdaptObjects");
                return DoAdaptObjects(std::move(originalObjects), result);
            }

            virtual TVector<TResultObject> DoDetachObjects() override {
                Y_ASSERT(!!OriginalReader);
                if (!OriginalReader) {
                    return TVector<TResultObject>();
                }
                TVector<TOriginalObject> objects = OriginalReader->DetachObjects();
                TVector<TResultObject> resultLocal;
                if (!AdaptObjects(std::move(objects), resultLocal)) {
                    TFLEventLog::Error("cannot adapt objects");
                    return std::move(TVector<TResultObject>());
                }
                return std::move(resultLocal);
            }
        public:
            IAdaptiveObjectsReader(typename IObjectsReader<TOriginalObject>::TPtr originalReader)
                : OriginalReader(originalReader)
            {

            }

            virtual bool GetHasMore() const override final {
                Y_ASSERT(!!OriginalReader);
                if (!OriginalReader) {
                    TFLEventLog::Error("not initialized original reader for HasMore method");
                    return false;
                }
                return OriginalReader->GetHasMore();
            }
            virtual void SetCountLimit(const ui32 value) override final {
                Y_ASSERT(!!OriginalReader);
                if (!OriginalReader) {
                    TFLEventLog::Error("not initialized original reader for SetCountLimit method");
                } else {
                    OriginalReader->SetCountLimit(value);
                }
            }
            virtual void DropCountLimit() override final {
                Y_ASSERT(!!OriginalReader);
                if (!OriginalReader) {
                    TFLEventLog::Error("not initialized original reader for DropCountLimit method");
                } else {
                    OriginalReader->DropCountLimit();
                }
            }
            virtual bool GetNoDataOnEmptyFilter() const override final {
                Y_ASSERT(!!OriginalReader);
                if (!OriginalReader) {
                    TFLEventLog::Error("not initialized original reader for GetNoDataOnEmptyFilter method");
                    return false;
                }
                return OriginalReader->GetNoDataOnEmptyFilter();
            }

            virtual void SetNoDataOnEmptyFilter(const bool value) override final {
                Y_ASSERT(!!OriginalReader);
                if (!OriginalReader) {
                    TFLEventLog::Error("not initialized original reader for SetNoDataOnEmptyFilter method");
                } else {
                    return OriginalReader->SetNoDataOnEmptyFilter(value);
                }
            }

            virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override final {
                Y_ASSERT(!!OriginalReader);
                if (!OriginalReader) {
                    TFLEventLog::Error("not initialized original reader for adaptation");
                    return false;
                }
                return OriginalReader->DeserializeFromJson(jsonInfo);
            }

            virtual bool Read(NStorage::ITransaction& tr, IExternalData::TPtr externalData = nullptr) override final {
                Y_ASSERT(!!OriginalReader);
                if (!OriginalReader) {
                    TFLEventLog::Error("cannot read from not initialized original reader");
                    return "";
                }
                if (!OriginalReader->Read(tr, externalData)) {
                    TFLEventLog::Error("original reader read method failed");
                    return false;
                }
                return true;
            }

            virtual TString SerializeCursorToString() const override {
                Y_ASSERT(!!OriginalReader);
                if (!OriginalReader) {
                    TFLEventLog::Error("not initialized original reader for adaptation cursor serialization");
                    return "";
                }
                return OriginalReader->SerializeCursorToString();
            }

        };
    }
}
