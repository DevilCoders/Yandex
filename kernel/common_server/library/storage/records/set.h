#pragma once
#include "record.h"
#include "abstract.h"
#include "t_record.h"

namespace NCS {
    namespace NStorage {
        class TRecordsSet {
        public:
            static TRecordsSet FromVector(const TVector<TTableRecord>& records);
            static TRecordsSet FromVector(const TVector<TTableRecordWT>& records);

            TSet<TString> GetAllFieldNames() const;

            TString BuildCondition(const NRequest::IExternalMethods& transaction) const;

            size_t size() const {
                return Records.size();
            }

            TVector<TTableRecord>::const_iterator begin() const {
                return Records.begin();
            }

            TVector<TTableRecord>::const_iterator end() const {
                return Records.end();
            }

            bool empty() const {
                return Records.empty();
            }

            bool AddRow(const TTableRecord& row) {
                Records.emplace_back(std::move(row));
                return true;
            }

            bool AddRows(const TRecordsSet& src) {
                for (auto&& i : src) {
                    Records.emplace_back(i);
                }
                return true;
            }

            const TVector<TTableRecord>& GetRecords() const {
                return Records;
            }

        private:
            TVector<TTableRecord> Records;
        };

        class TRecordsSetWT: public IRecordsSetWT {
        private:
            TVector<TTableRecordWT> Records;
        public:
            virtual bool AddRow(TTableRecordWT&& row) override {
                Records.emplace_back(std::move(row));
                return true;
            }

            virtual void Reserve(const ui32 count) override {
                Records.reserve(count);
            }

            const TTableRecordWT& front() const {
                return Records.front();
            }

            size_t size() const {
                return Records.size();
            }

            bool empty() const {
                return Records.empty();
            }

            TSet<TString> SelectSet(const TString& fieldId) const;

            TVector<TTableRecordWT>::const_iterator begin() const {
                return Records.begin();
            }

            TVector<TTableRecordWT>::const_iterator end() const {
                return Records.end();
            }

        };

    }
}

