#pragma once
#include "object.h"
#include <kernel/common_server/api/history/common.h>
#include <kernel/common_server/library/storage/records/t_record.h>
#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/roles/actions/common.h>
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/library/storage/selection/filter.h>

namespace NCS {
    namespace NTags {
        class TTagsSelectionExternalData: public NCS::NSelection::IExternalData {
        private:
            CSA_READONLY_DEF(IUserPermissions::TPtr, UserPermissions);
        public:
            TTagsSelectionExternalData(IUserPermissions::TPtr permissions)
                : UserPermissions(permissions)
            {

            }
        };

        class TFilterByTagNames: public NCS::NSelection::IFilter {
        private:
            using TBase = NCS::NSelection::IFilter;
            CSA_MAYBE(TFilterByTagNames, TSet<TString>, TagNames);
        protected:
            virtual void FillFilter(TSRMulti& srMulti, NSelection::IExternalData::TPtr externalData) const override;
            virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
                if (!TJsonProcessor::ReadContainer(jsonInfo, "tag_names", TagNames)) {
                    return false;
                }
                return true;
            }
            virtual void FillScheme(NCS::NScheme::TScheme& scheme) const override {
                scheme.Add<TFSArray>("tag_names").InitElement<TFSString>().SetRequired(false);
            }
        };

        class TObjectsFilterByTagNames: public NCS::NSelection::IFilter {
        private:
            using TBase = NCS::NSelection::IFilter;
            CSA_MAYBE(TObjectsFilterByTagNames, TSet<TString>, TagNames);
        protected:
            virtual void FillFilter(TSRMulti& srMulti, NSelection::IExternalData::TPtr externalData) const override;
            virtual bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
                if (!TJsonProcessor::ReadContainer(jsonInfo, "tag_names", TagNames)) {
                    return false;
                }
                return true;
            }
            virtual void FillScheme(NCS::NScheme::TScheme& scheme) const override {
                scheme.Add<TFSArray>("tag_names").InitElement<TFSString>().SetRequired(false);
            }
        };

        class TFilter: public NSelection::NFilter::TComposite {
        public:
            TFilter() {
                Register<NCS::NSelection::NFilter::TObjectIds>().SetObjectIdFieldName("object_id");
                Register<TFilterByTagNames>();
            }
        };

        class TObjectsFilter: public NSelection::NFilter::TComposite {
        public:
            TObjectsFilter() {
                Register<NCS::NSelection::NFilter::TObjectIds>().SetObjectIdFieldName("object_id");
                Register<TObjectsFilterByTagNames>();
            }
        };

        class TSorting: public NSelection::NSorting::TLinear {
        public:
            TSorting() {
                RegisterField("object_id").RegisterField("tag_id");
            }
        };

        using TSelection = NSelection::TSelection<TFilter, TSorting>;
        using TObjectsSelection = NSelection::TSelection<TObjectsFilter, TSorting>;

    }
}
