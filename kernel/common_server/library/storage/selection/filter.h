#pragma once

#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/library/storage/query/request.h>
#include <kernel/common_server/util/cgi_processing.h>
#include <kernel/common_server/library/scheme/scheme.h>
#include "abstract.h"

namespace NCS {
    namespace NSelection {
        namespace NFilter {

            class TInstantInterval: public IFilter {
            private:
                CSA_DEFAULT(TInstantInterval, TString, InstantFieldName);
                CSA_MAYBE(TInstantInterval, TInstant, InstantFrom);
                CSA_MAYBE(TInstantInterval, TInstant, InstantTo);
            public:
                virtual void FillFilter(TSRMulti& srMulti, IExternalData::TPtr /*externalData*/) const override {
                    if (HasInstantFrom()) {
                        srMulti.InitNode<TSRBinary>(InstantFieldName, GetInstantFromUnsafe().Seconds(), ESRBinary::GreaterOrEqual);
                    }
                    if (HasInstantTo()) {
                        srMulti.InitNode<TSRBinary>(InstantFieldName, GetInstantToUnsafe().Seconds(), ESRBinary::Less);
                    }
                }

                virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
                    return TJsonProcessor::Read(jsonInfo, "instant_from", InstantFrom)
                        && TJsonProcessor::Read(jsonInfo, "instant_to", InstantTo);
                }

                virtual void FillScheme(NCS::NScheme::TScheme& scheme) const override {
                    scheme.Add<TFSString>("instant_field_name").SetDefault(InstantFieldName).SetRequired(true).ReadOnly();
                    scheme.Add<TFSNumeric>("instant_from").SetRequired(false);
                    scheme.Add<TFSNumeric>("instant_to").SetRequired(false);
                }
            };

            class TObjectIds: public IFilter {
            private:
                CSA_DEFAULT(TObjectIds, TString, ObjectIdFieldName);
                CSA_READONLY_DEF(TSet<TString>, ObjectIds);
            public:
                virtual void FillFilter(TSRMulti& srMulti, IExternalData::TPtr /*externalData*/) const override {
                    if (GetObjectIds().empty()) {
                        return;
                    }
                    srMulti.InitNode<TSRBinary>(ObjectIdFieldName, GetObjectIds());
                }

                virtual void FillScheme(NCS::NScheme::TScheme& scheme) const override {
                    scheme.Add<TFSString>("object_id_field_name").SetDefault(ObjectIdFieldName).SetRequired(true).ReadOnly();
                    scheme.Add<TFSArray>("object_id").SetElement<TFSString>();
                }

                virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
                    if (!TJsonProcessor::ReadContainer(jsonInfo, "object_id", ObjectIds, false)) {
                        return false;
                    }
                    return true;
                }
            };

            class TComposite: public IFilter {
            private:
                TVector<IFilter::TPtr> Filters;
            protected:
                template <class T>
                T& Register() {
                    TAtomicSharedPtr<T> newObject = new T;
                    Filters.emplace_back(newObject);
                    return *newObject;
                }
            public:
                template <class T>
                TAtomicSharedPtr<T> GetFilter() const {
                    for (auto&& i : Filters) {
                        auto result = DynamicPointerCast<T>(i);
                        if (!!result) {
                            return result;
                        }
                    }
                    return nullptr;
                }

                template <class T>
                TVector<TAtomicSharedPtr<T>> GetFilters() const {
                    TVector<TAtomicSharedPtr<T>> result;
                    for (auto&& i : Filters) {
                        auto resultFilter = DynamicPointerCast<T>(i);
                        if (!!resultFilter) {
                            result.emplace_back(resultFilter);
                            continue;
                        }
                        auto resultComposite = DynamicPointerCast<TComposite>(i);
                        if (!!resultComposite) {
                            TVector<TAtomicSharedPtr<T>> localFilters = resultComposite->GetFilters<T>();
                            result.insert(result.end(), localFilters.begin(), localFilters.end());
                        }
                    }
                    return result;
                }

                virtual void FillFilter(TSRMulti& srMulti, IExternalData::TPtr externalData) const override {
                    for (auto&& i : Filters) {
                        i->FillFilter(srMulti, externalData);
                    }
                }

                virtual void FillScheme(NCS::NScheme::TScheme& scheme) const override {
                    for (auto&& i : Filters) {
                        i->FillScheme(scheme);
                    }
                }

                virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
                    for (auto&& i : Filters) {
                        if (!i->DeserializeFromJson(jsonInfo)) {
                            return false;
                        }
                    }
                    return true;
                }
            };
        }
    }
}
