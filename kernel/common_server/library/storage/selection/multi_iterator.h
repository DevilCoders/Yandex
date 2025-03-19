#pragma once

#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/library/storage/query/request.h>
#include <kernel/common_server/library/storage/selection/selection.h>
#include <kernel/common_server/util/cgi_processing.h>
#include <kernel/common_server/library/storage/abstract/database.h>
#include <kernel/common_server/library/storage/reply/parsed.h>
#include "abstract.h"
#include "iterator.h"
#include "sorting.h"

namespace NCS {
    namespace NSelection {
        template <class TDBObject, class TSelectionExt>
        class TMultiReader: public IBaseObjectsReader<TDBObject> {
        private:
            using TBase = IBaseObjectsReader<TDBObject>;
            using TReader = TReader<TDBObject, TSelectionExt>;
            using TReaders = TVector<TAtomicSharedPtr<TReader>>;
            CSA_READONLY_DEF(TReaders, Readers);

            class TReservedObjectsInfo {
            private:
                CSA_DEFAULT(TReservedObjectsInfo, TDeque<TDBObject>, Objects);
                CSA_MAYBE(TReservedObjectsInfo, TDBObject, LastUsageObject);
                CS_ACCESS(TReservedObjectsInfo, bool, HasMore, false);
            public:
            };

            TMap<TString, TReservedObjectsInfo> ReservedObjects;
            CSA_READONLY_DEF(TVector<TDBObject>, Objects);
            TAtomicSharedPtr<NSorting::TLinear> SortingInfo;
            bool FillSortingInfo() {
                if (!SortingInfo) {
                    for (auto&& i : Readers) {
                        if (i->GetHasMore()) {
                            SortingInfo = i->GetSortingPtr();
                            HasMore = true;
                            return true;
                        }
                    }
                }
                SortingInfo = new NSorting::TLinear;
                return false;
            }
        public:
            using TBase::GetNoDataOnEmptyFilter;
        protected:
            using TBase::HasMore;
            using TBase::CountLimit;

            virtual bool DoDeserializeFromJson(const NJson::TJsonValue& jsonInfo) override {
                THolder<NSorting::TCompositeCursor> cursorLocal;
                if (jsonInfo.Has("cursor")) {
                    cursorLocal = MakeHolder<NSorting::TCompositeCursor>();
                    if (!cursorLocal->DeserializeFromString(jsonInfo["cursor"].GetStringRobust())) {
                        TFLEventLog::Error("cannot parse sorting cursor for selection");
                        return false;
                    }
                }
                for (auto&& i : Readers) {
                    if (!i->DeserializeFromJson(jsonInfo)) {
                        TFLEventLog::Error("cannot parse filter for selection");
                        return false;
                    }
                    if (cursorLocal) {
                        i->SetCursor(cursorLocal->GetCursor(i->GetTableName()));
                    }
                }
                if (cursorLocal) {
                    const NSorting::TCompositeCursor* cursorLocalPointer = cursorLocal.Get();
                    const auto pred = [cursorLocalPointer](const TAtomicSharedPtr<TReader>& item) {
                        return !item || !cursorLocalPointer->GetCursor(item->GetTableName());
                    };
                    Readers.erase(std::remove_if(Readers.begin(), Readers.end(), pred), Readers.end());
                }
                FillSortingInfo();
                return true;
            }
            class TObjectsIterator {
            private:
                TReservedObjectsInfo* ObjectsInfo;
                TDeque<TDBObject>* Objects;
                TAtomicSharedPtr<NSelection::NSorting::TLinear> Sorting;
            public:
                TObjectsIterator() = default;
                TObjectsIterator(TReservedObjectsInfo& objectsInfo, TAtomicSharedPtr<NSelection::NSorting::TLinear> sorting)
                    : ObjectsInfo(&objectsInfo)
                    , Objects(&objectsInfo.MutableObjects())
                    , Sorting(sorting)
                {
                    Y_ASSERT(sorting);
                }

                TDBObject DetachAndNext() {
                    CHECK_WITH_LOG(Objects->size());
                    TDBObject result = std::move(Objects->front());
                    Objects->pop_front();
                    ObjectsInfo->SetLastUsageObject(result);
                    return std::move(result);
                }

                bool IsFinished() const {
                    return Objects->empty();
                }

                bool operator<(const TObjectsIterator& value) const {
                    if (Sorting) {
                        return !Sorting->Compare(this->Objects->front(), value.Objects->front());
                    } else {
                        return (ui64)this < (ui64)&value;
                    }
                }
            };

            virtual TVector<TDBObject> DoDetachObjects() override {
                return std::move(Objects);
            }

        public:
            TMultiReader() = default;

            bool Register(typename IObjectsReader<TDBObject>::TPtr reader) {
                if (!!SortingInfo) {
                    TFLEventLog::Error("cannot add readers after sorting prepared");
                    return false;
                }
                auto readerInternal = DynamicPointerCast<TReader>(reader);
                Y_ASSERT(!!readerInternal);
                if (!readerInternal) {
                    TFLEventLog::Error("incorrect reader class");
                    return false;
                }
                Readers.emplace_back(readerInternal);
                return true;
            }

            virtual TString SerializeCursorToString() const override {
                if (!HasMore) {
                    return "";
                } else {
                    THolder<NSorting::TCompositeCursor> cursor(new NSorting::TCompositeCursor);
                    for (auto&& i : Readers) {
                        auto it = ReservedObjects.find(i->GetTableName());
                        if (it == ReservedObjects.end() || it->second.GetObjects().empty() || !it->second.HasLastUsageObject()) {
                            if (!i->GetCursor()) {
                                continue;
                            } else {
                                cursor->RegisterCursor(*i->GetCursor());
                            }
                        } else {
                            NSorting::TSimpleCursor sCursor(i->GetTableName());
                            if (!!SortingInfo) {
                                sCursor.FillFromSorting(*SortingInfo);
                            }
                            if (!sCursor.FillCursor(it->second.GetLastUsageObjectUnsafe().SerializeToTableRecord().BuildWT())) {
                                TFLEventLog::Error("cannot read cursor fields");
                                return "";
                            }
                            cursor->RegisterCursor(sCursor);
                        }
                    }
                    return cursor->SerializeToString();
                }
            }

            virtual bool Read(NStorage::ITransaction& tr, IExternalData::TPtr externalData = nullptr) override {
                if (Readers.empty()) {
                    Objects.clear();
                    return true;
                }
                if (!SortingInfo->GetFields().size()) {
                    TFLEventLog::Error("not found sorting rules");
                    return false;
                }
                HasMore = false;
                for (auto&& i : Readers) {
                    if (!i->GetHasMore()) {
                        continue;
                    }
                    ui32 reservedObjects = 0;
                    auto it = ReservedObjects.find(i->GetTableName());
                    if (it != ReservedObjects.end()) {
                        reservedObjects = it->second.GetObjects().size();
                    }
                    i->SetNoDataOnEmptyFilter(GetNoDataOnEmptyFilter());
                    if (CountLimit) {
                        if (*CountLimit > reservedObjects) {
                            i->SetCountLimit(*CountLimit - reservedObjects);
                        } else {
                            if (it->second.GetHasMore()) {
                                HasMore = true;
                            }
                            continue;
                        }
                    } else {
                        i->DropCountLimit();
                    }
                    if (!i->Read(tr, externalData)) {
                        TFLEventLog::Error("cannot execute cursor reader");
                        return false;
                    }
                    if (it == ReservedObjects.end()) {
                        it = ReservedObjects.emplace(i->GetTableName(), TReservedObjectsInfo()).first;
                    }
                    auto objectsLocal = i->DetachObjects();
                    it->second.MutableObjects().insert(it->second.MutableObjects().end(), objectsLocal.begin(), objectsLocal.end());
                    it->second.SetHasMore(i->GetHasMore());
                    if (i->GetHasMore()) {
                        HasMore = true;
                    }
                }
                TVector<TObjectsIterator> iterators;
                for (auto&& i : ReservedObjects) {
                    if (i.second.GetObjects().empty()) {
                        continue;
                    }
                    iterators.emplace_back(TObjectsIterator(i.second, SortingInfo));
                }
                MakeHeap(iterators.begin(), iterators.end());
                TVector<TDBObject> objects;
                while (iterators.size()) {
                    PopHeap(iterators.begin(), iterators.end());
                    objects.emplace_back(iterators.back().DetachAndNext());
                    if (iterators.back().IsFinished()) {
                        iterators.pop_back();
                    } else {
                        PushHeap(iterators.begin(), iterators.end());
                    }
                    if (CountLimit && *CountLimit == objects.size()) {
                        break;
                    }
                }
                if (!HasMore) {
                    for (auto&& i : ReservedObjects) {
                        if (i.second.GetObjects().size()) {
                            HasMore = true;
                            break;
                        }
                    }
                }
                std::swap(Objects, objects);
                return true;
            }
        };

    }
}
