#pragma once
#include <kernel/common_server/library/storage/records/abstract.h>
#include <kernel/common_server/util/accessor.h>
#include <util/generic/vector.h>
#include <util/generic/map.h>

namespace NCS {
    namespace NStorage {
        class IOriginalContainer: TNonCopyable {
        public:
            virtual ~IOriginalContainer() = default;
        };

        class TOrderedColumn {
        private:
            CSA_READONLY_DEF(TString, Name);
            CSA_READONLY(EColumnType, Type, EColumnType::Text);
        public:
            TOrderedColumn(const TString& name, const EColumnType type)
                : Name(name)
                , Type(type) {

            }

            static TMap<TString, ui32> BuildRemap(const TVector<TOrderedColumn>& columns) {
                TMap<TString, ui32> remap;
                ui32 idx = 0;
                for (auto&& i : columns) {
                    remap.emplace(i.GetName(), idx++);
                }
                return remap;
            }
        };

        class IPackedRecordsSet: public IBaseRecordsSet {
        protected:
            TVector<TOrderedColumn> OrderedColumns;
            ui32 RecordsCount = 0;
            ui32 CurrentRecord = 0;
            ui32 ColumnsCount = 0;
        public:

            ui32 GetRecordsCount() const {
                return RecordsCount;
            }

            const TVector<TOrderedColumn>& GetOrderedColumns() const {
                return OrderedColumns;
            }

            virtual void Initialize(const ui32 recordsCount, const TVector<TOrderedColumn>& orderedColumns) {
                OrderedColumns = orderedColumns;
                ColumnsCount = orderedColumns.size();
                RecordsCount = recordsCount;
            }

            virtual IPackedRecordsSet& StoreOriginalData(THolder<IOriginalContainer>&& /*data*/) {
                return *this;
            }

            virtual void AddRow(const TVector<TStringBuf>& values);
        };

        class TPackedRecordsSet: public IPackedRecordsSet {
        private:
            TVector<TVector<TStringBuf>> Values;
        protected:
            THolder<IOriginalContainer> OriginalData;
        public:

            virtual IPackedRecordsSet& StoreOriginalData(THolder<IOriginalContainer>&& data) override;

            TPackedRecordsSet() = default;

            TVector<TVector<TStringBuf>>::const_iterator begin() const {
                return Values.begin();
            }

            TVector<TVector<TStringBuf>>::const_iterator end() const {
                return Values.end();
            }

            virtual void AddRow(const TVector<TStringBuf>& values) override {
                IPackedRecordsSet::AddRow(values);
                Values.emplace_back(values);
            }

            virtual void Initialize(const ui32 recordsCount, const TVector<TOrderedColumn>& remapColumns) override {
                IPackedRecordsSet::Initialize(recordsCount, remapColumns);
                Values.reserve(RecordsCount);
            }
        };

        class IParsingFailPolicy {
        public:
            using TPtr = TAtomicSharedPtr<IParsingFailPolicy>;
            virtual ~IParsingFailPolicy() = default;
            virtual void OnFail(const TVector<TStringBuf>& values) = 0;
        };

        class TPanicOnFailPolicy: public IParsingFailPolicy {
        private:
            const TString Header;
        public:
            TPanicOnFailPolicy() = default;
            TPanicOnFailPolicy(const TString& header)
                : Header(header) {

            }

            virtual void OnFail(const TVector<TStringBuf>& values) override;
        };

    }
}
