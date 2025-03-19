#pragma once

#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/library/storage/query/request.h>
#include <kernel/common_server/util/cgi_processing.h>
#include "filter.h"

namespace NCS {
    namespace NSelection {
        class ISelection {
        protected:
            virtual void DoFillScheme(NCS::NScheme::TScheme& scheme) const = 0;
        public:
            virtual ~ISelection() = default;
            void FillScheme(NCS::NScheme::TScheme& scheme) const {
                DoFillScheme(scheme);
            }

            NCS::NScheme::TScheme GetScheme() const {
                NCS::NScheme::TScheme result;
                FillScheme(result);
                return result;
            }
        };

        template <class TObject>
        class IObjectSelection: public ISelection {
        protected:
            virtual typename IObjectsReader<TObject>::TPtr DoBuildReader() const = 0;
            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) = 0;
        public:
            using TPtr = TAtomicSharedPtr<IObjectSelection>;
            typename IObjectsReader<TObject>::TPtr BuildReader() const {
                return DoBuildReader();
            }
            bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
                return DoDeserializeFromJson(jsonInfo);
            }
        };

        template <class TObject>
        class TCompositeSelection: public IObjectSelection<TObject> {
        private:
            TMap<TString, typename IObjectSelection<TObject>::TPtr> Selections;
            TString SelectionId;
            virtual void DoFillScheme(NCS::NScheme::TScheme& scheme) const override {
                if (Selections.size()) {
                    auto& wVariants = scheme.Add<TFSWideVariants>("selection_id");
                    for (auto&& i : Selections) {
                        TFSWideVariants::TVariant v;
                        v.SetRealValue(i.first);
                        NCS::NScheme::TScheme scheme;
                        i.second->FillScheme(scheme);
                        v.SetAdditionalScheme(scheme);
                        wVariants.MutableVariants().emplace_back(v);
                    }
                }
            }
        protected:
            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
                if (!TJsonProcessor::Read(jsonInfo, "selection_id", SelectionId)) {
                    return false;
                }
                auto it = Selections.find(SelectionId);
                if (it == Selections.end()) {
                    TFLEventLog::Error("incorrect selection_id")("selection_id", SelectionId);
                    return false;
                }
                return it->second->DeserializeFromJson(jsonInfo);
            }
            virtual typename IObjectsReader<TObject>::TPtr DoBuildReader() const override {
                auto it = Selections.find(SelectionId);
                if (it == Selections.end()) {
                    TFLEventLog::Error("incorrect selection_id on build reader")("selection_id", SelectionId);
                    return nullptr;
                }
                return it->second->BuildReader();
            }
        public:
            TCompositeSelection& Register(const TString& selectionId, typename IObjectSelection<TObject>::TPtr selection) {
                const bool success = Selections.emplace(selectionId, selection).second;
                Y_ASSERT(success);
                return *this;
            }
        };

        template <class TObject, class TFilterExt, class TSortingExt>
        class TFullSelection: public IObjectSelection<TObject> {
        public:
            using TFilter = TFilterExt;
            using TSorting = TSortingExt;
        protected:
            TFilter Filter;
            TSorting Sorting;

            virtual void DoFillScheme(NCS::NScheme::TScheme& scheme) const override {
                scheme.Add<TFSNumeric>("count_limit").SetDefault(100);
                Filter.FillScheme(scheme);
                Sorting.FillScheme(scheme);
            }

            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
                if (!Filter.DeserializeFromJson(jsonInfo)) {
                    TFLEventLog::Error("cannot parse filter for selection");
                    return false;
                }
                if (!Sorting.DeserializeFromJson(jsonInfo)) {
                    TFLEventLog::Error("cannot parse sorting for selection");
                    return false;
                }
                return true;
            }
        public:

            static NCS::NScheme::TScheme GetScheme() {
                NCS::NScheme::TScheme result;
                TFullSelection().FillScheme(result);
                return result;
            }

            template <class T>
            static NCS::NScheme::TScheme GetSearchScheme(const T& /*server*/) {
                return GetScheme();
            }

        };

        template <class TFilterExt, class TSortingExt>
        class TSelection {
        public:
            using TFilter = TFilterExt;
            using TSorting = TSortingExt;
        protected:
            TFilter Filter;
            TSorting Sorting;
        public:

            template <class T>
            TAtomicSharedPtr<T> GetFilter() {
                return Filter.template GetFilter<T>();
            }

            static NCS::NScheme::TScheme GetScheme() {
                NCS::NScheme::TScheme result;
                result.Add<TFSNumeric>("count_limit").SetDefault(100);
                TFilter().FillScheme(result);
                TSorting().FillScheme(result);
                return result;
            }

            template <class T>
            static NCS::NScheme::TScheme GetSearchScheme(const T& /*server*/) {
                return GetScheme();
            }

        };
    }
}
