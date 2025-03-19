#pragma once

#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/library/storage/query/request.h>
#include <kernel/common_server/library/storage/selection/selection.h>
#include <kernel/common_server/util/cgi_processing.h>
#include <kernel/common_server/library/storage/abstract/database.h>
#include <kernel/common_server/library/storage/reply/parsed.h>
#include "abstract.h"
#include "sorting.h"
#include "cursor.h"

namespace NCS {
    namespace NSelection {
        template <class TDBObject, class TSelectionExt>
        class TReader: public IBaseObjectsReader<TDBObject> {
        private:
            using TBase = IBaseObjectsReader<TDBObject>;
        public:
            using TFilter = typename TSelectionExt::TFilter;
            using TSorting = typename TSelectionExt::TSorting;
            using TBase::GetNoDataOnEmptyFilter;
        protected:
            using TBase::HasMore;
            using TBase::CountLimit;
        private:
            TAtomicSharedPtr<TFilter> Filter;
            TAtomicSharedPtr<TSorting> Sorting;
            TAtomicSharedPtr<NSorting::TSimpleCursor> Cursor;
            CSA_READONLY_DEF(TString, TableName);
            CSA_READONLY_DEF(NStorage::TObjectRecordsSet<TDBObject>, Objects);

            bool FillQuery(TSRSelect& srSelect, IExternalData::TPtr externalData) const {
                TSRCondition srCondition;
                TSRMulti& srMulti = srCondition.Ret<TSRMulti>();
                if (Filter) {
                    Filter->FillFilter(srMulti, externalData);
                }
                if (!srMulti.IsEmpty()) {
                    srSelect.SetCondition(srCondition);
                } else if (GetNoDataOnEmptyFilter()) {
                    srSelect.InitCondition<TSRValue>(false);
                } else if (!CountLimit) {
                    srSelect.InitCondition<TSRValue>(false);
                }
                if (Cursor) {
                    Cursor->FillSorting(srSelect);
                } else if (Sorting) {
                    Sorting->FillSorting(srSelect);
                }
                return true;
            }
        protected:
            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
                {
                    Filter = new TFilter;
                    if (!Filter->DeserializeFromJson(jsonInfo)) {
                        TFLEventLog::Error("cannot parse filter for selection");
                        return false;
                    }
                }
                if (jsonInfo.Has("cursor")) {
                    THolder<NSorting::TSimpleCursor> cursorLocal(new NSorting::TSimpleCursor(TableName));
                    if (!cursorLocal->DeserializeFromString(jsonInfo["cursor"].GetStringRobust())) {
                        TFLEventLog::Error("cannot parse sorting cursor for selection");
                        return false;
                    }
                    if (cursorLocal->GetTableName() != TableName) {
                        TFLEventLog::Error("incompatible cursor tablename")("expected", TableName)("actual", cursorLocal->GetTableName());
                        return false;
                    }
                    Cursor = cursorLocal.Release();
                    Sorting = nullptr;
                } else if (jsonInfo.Has("sort_fields")) {
                    THolder<TSorting> localSorting(new TSorting);
                    if (!localSorting->DeserializeFromJson(jsonInfo)) {
                        TFLEventLog::Error("cannot parse sorting for selection");
                        return false;
                    }
                    Sorting = localSorting.Release();
                    Cursor = nullptr;
                } else {
                    Sorting = new TSorting;
                    Cursor = nullptr;
                }
                return true;
            }

            void DropCursor() {
                Cursor = nullptr;
            }

            bool FillCursor(const NCS::NStorage::TTableRecordWT& tr) {
                THolder<NSorting::TSimpleCursor> sc(new NSorting::TSimpleCursor(TableName));
                if (Sorting) {
                    sc->FillFromSorting(*Sorting);
                } else if (Cursor) {
                    *sc = *Cursor;
                }
                if (!sc->FillCursor(tr)) {
                    TFLEventLog::Error("cannot fill cursor");
                    return false;
                }
                Cursor = sc.Release();
                Sorting = nullptr;
                return true;
            }

            virtual TVector<TDBObject> DoDetachObjects() override {
                return Objects.DetachObjects();
            }

        public:
            TReader(const TString& tableName)
                : TableName(tableName) {
                Filter = new TFilter;
                Sorting = new TSorting;
            }

            template <class T>
            TAtomicSharedPtr<T> GetFilter() {
                return Filter->template GetFilter<T>();
            }

            virtual TString SerializeCursorToString() const override {
                if (!Cursor) {
                    return "";
                } else {
                    return Cursor->SerializeToString();
                }
            }

            TAtomicSharedPtr<NCS::NSelection::NSorting::TLinear> GetSortingPtr() const {
                if (Sorting) {
                    return Sorting;
                } else if (Cursor) {
                    return Cursor->BuildSorting();
                } else {
                    return new NCS::NSelection::NSorting::TLinear;
                }
            }

            TAtomicSharedPtr<NSorting::TSimpleCursor> GetCursor() const {
                return Cursor;
            }

            template <class TPolicy>
            void SetCursor(const TMaybe<NSorting::TSimpleCursor, TPolicy>& sc) {
                if (!sc) {
                    Cursor = nullptr;
                    Sorting = nullptr;
                } else {
                    Cursor = new NSorting::TSimpleCursor(*sc);
                    Sorting = nullptr;
                }
            }

            virtual bool Read(NStorage::ITransaction& tr, IExternalData::TPtr externalData = nullptr) override {
                NCS::NStorage::TObjectRecordsSet<TDBObject> objectsLocal;
                if (HasMore) {
                    TSRSelect srSelect(TableName, &objectsLocal);
                    FillQuery(srSelect, externalData);
                    if (CountLimit) {
                        srSelect.SetCountLimit(*CountLimit + 1);
                    }
                    if (!tr.ExecRequest(srSelect)->IsSucceed()) {
                        return false;
                    }
                    HasMore = CountLimit && (objectsLocal.size() > *CountLimit);
                    if (HasMore) {
                        objectsLocal.pop_back();
                    }
                }
                std::swap(objectsLocal, Objects);
                if (Objects.size()) {
                    if (!FillCursor(Objects.back().SerializeToTableRecord().BuildWT())) {
                        TFLEventLog::Error("cannot fill reader cursor");
                        return false;
                    }
                } else {
                    DropCursor();
                }
                return true;
            }

        };
    }
}
