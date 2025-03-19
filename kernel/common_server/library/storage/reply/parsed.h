#pragma once
#include "abstract.h"
#include <kernel/common_server/util/accessor.h>
#include <library/cpp/logger/global/global.h>
#include <kernel/common_server/library/logging/events.h>
#include <util/string/join.h>
#include <util/generic/map.h>
#include <util/generic/algorithm.h>

namespace NCS {
    namespace NStorage {
        template <class T, class TContext = TNull>
        class TObjectRecordsSet: public IPackedRecordsSet {
        private:
            using TBase = IPackedRecordsSet;
            using TDecoder = typename T::TDecoder;
            TVector<T> Objects;
            CSA_DEFAULT(TObjectRecordsSet, IParsingFailPolicy::TPtr, ParsingFailPolicy);
        protected:
            TDecoder Decoder;
            const TContext* Context = nullptr;
        public:

            bool empty() const {
                return !size();
            }

            using TBase::GetRecordsCount;

            TObjectRecordsSet() = default;

            TObjectRecordsSet(const TContext* context)
                : Context(context) {

            }

            const TVector<T>& GetObjects() const {
                return Objects;
            }

            TVector<T>&& DetachObjects() {
                return std::move(Objects);
            }

            const T& operator[](const size_t idx) const {
                return Objects[idx];
            }

            typename TVector<T>::const_iterator begin() const {
                return Objects.begin();
            }

            typename TVector<T>::const_iterator end() const {
                return Objects.end();
            }

            typename TVector<T>::iterator begin() {
                return Objects.begin();
            }

            typename TVector<T>::iterator end() {
                return Objects.end();
            }

            void pop_back() {
                if (Objects.size()) {
                    Objects.pop_back();
                }
            }

            const T& front() const {
                return Objects.front();
            }

            const T& back() const {
                return Objects.back();
            }

            T& front() {
                return Objects.front();
            }

            T& back() {
                return Objects.back();
            }

            size_t size() const {
                return Objects.size();
            }

            template <class P>
            void EraseIf(P&& pred) {
                ::EraseIf(Objects, pred);
                RecordsCount = Objects.size();
            }

            virtual void AddRow(const TVector<TStringBuf>& values) override {
                IPackedRecordsSet::AddRow(values);
                T object;
                try {
                    if (object.DeserializeWithDecoder(Decoder, values)) {
                        Objects.emplace_back(std::move(object));
                    } else {
                        TStringStream ss;
                        ss << "Cannot parse object type " << TypeName<T>() << " for record " << JoinSeq(", ", values);
                        if (ParsingFailPolicy) {
                            ParsingFailPolicy->OnFail(values);
                        }
                        ++ErrorsCount;
                        TFLEventLog::Error("cannot parse record from database")("object_type", TypeName<T>())("record", JoinSeq(", ", values));
                    }
                } catch (...) {
                    ++ErrorsCount;
                    TFLEventLog::Error("exception on database record parsing")("object_type", TypeName<T>())("record", JoinSeq(", ", values))("exception", CurrentExceptionMessage());
                }
            }

            virtual void Initialize(const ui32 recordsCount, const TVector<TOrderedColumn>& remapColumns) override {
                IPackedRecordsSet::Initialize(recordsCount, remapColumns);
                TMap<TString, ui32> decodeRemapper;
                ui32 idx = 0;
                for (auto&& i : remapColumns) {
                    decodeRemapper[i.GetName()] = idx++;
                }
                Decoder = TDecoder(decodeRemapper);
                Decoder.TakeColumnsInfo(remapColumns);
                Objects.reserve(RecordsCount);
            }
        };

    }
}
