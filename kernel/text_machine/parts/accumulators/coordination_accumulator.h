#pragma once

#include <kernel/text_machine/parts/common/storage.h>
#include <kernel/text_machine/module/save_to_json.h>

#include <util/generic/vector.h>
#include <util/generic/ymath.h>
#include <util/generic/ylimits.h>
#include <util/system/defaults.h>
#include <util/system/yassert.h>

namespace NTextMachine {
namespace NCore {
    class TCoordinationAccumulator : public NModule::TJsonSerializable {
    public:
        void Init(TMemoryPool& pool, ui16 queryWordCount);
        void Clear();
        void Update(ui16 wordId);
        bool Contain(ui16 wordId) const;
        size_t Count() const;

        void SaveToJson(NJson::TJsonValue& value) const;

    private:
        size_t Id = Max<size_t>();
        TPoolPodHolder<size_t> LastIds;
        size_t MatchCount = Max<size_t>();
    };

    Y_FORCE_INLINE void TCoordinationAccumulator::Update(ui16 wordId)
    {
        Y_ASSERT(Id > 0);
        if (LastIds[wordId] != Id) {
            LastIds[wordId] = Id;
            ++MatchCount;
            Y_ASSERT(MatchCount <= LastIds.Count());
        }
    }

    Y_FORCE_INLINE void TCoordinationAccumulator::Clear()
    {
        ++Id;
        MatchCount = 0;
    }

    Y_FORCE_INLINE bool TCoordinationAccumulator::Contain(ui16 wordId) const
    {
        Y_ASSERT(Id > 0);
        return Id == LastIds[wordId];
    }

    Y_FORCE_INLINE size_t TCoordinationAccumulator::Count() const
    {
        return MatchCount;
    }

    template<typename TData, typename TIndexType = ui16, typename TGenerationType = ui32>
    class TGenerativeOptionalArrayAccumulator: public NModule::TJsonSerializable {
    private:
        struct TNode {
            TGenerationType Generation;
            TIndexType Next;
            TData Value;
        };
        bool MarkSlotAsNonEmpty(TIndexType id) {
            TNode& cur = Data[id];
            if (cur.Generation != GenerationId) {
                cur.Generation = GenerationId;
                cur.Next = HeadPos;
                HeadPos = id;

                NumNonEmpties += 1;
                return true;
            }
            return false;
        }
    public:
        void Init(TMemoryPool& pool, TIndexType numObjects) {
            Data.Init(pool, numObjects + 1, EStorageMode::Full);
            NumObjects = numObjects;
            Data.FillZeroes();
            GenerationId = 0;
            Clear();
        }

        //mark container as empty
        inline void Clear() {
            GenerationId += 1;
            HeadPos = NumObjects;
            NumNonEmpties = 0;
            Y_ASSERT(Data[HeadPos].Generation != GenerationId);
            Y_ASSERT(GenerationId < Max<TGenerationType>());
        }

        //return true if data was set, false if slot was not empty
        template<class T>
        bool SetIfEmpty(TIndexType id, T&& d) {
            Y_ASSERT(id < NumObjects);
            bool res = MarkSlotAsNonEmpty(id);
            if (res) {
                Data[id].Value = std::forward<T>(d);
            }
            return res;
        }

        //return true if data was set at first time
        template<class T>
        bool Set(TIndexType id, T&& d) {
            Y_ASSERT(id < NumObjects);
            Data[id].Value = std::forward<T>(d);
            return MarkSlotAsNonEmpty(id);
        }

        //return Data-ref by index, if data is not toched yet will set it to given value
        template<class T = TData>
        TData& GetRefWithDefault(TIndexType id, T&& d) {
            SetIfEmpty(id, std::forward<T>(d));
            return Data[id].Value;
        }

        template<class T = TData>
        TData GetValWithDefault(TIndexType id, T&& d) const {
            const TNode& cur = Data[id];
            if (cur.Generation != GenerationId) {
                return std::forward<T>(d);
            }
            return Data[id].Value;
        }

        bool Contain(TIndexType id) const {
            Y_ASSERT(id < NumObjects);
            return Data[id].Generation == GenerationId;
        }

        struct TEndIterTag {};

        class TIter {
            const TGenerativeOptionalArrayAccumulator* P;
            ui32 Pos = 0;
            ui32 Gen = 0;
        public:
            TIter(const TGenerativeOptionalArrayAccumulator& p)
                : P(&p)
            {
                Gen = P->GenerationId;
                Pos = P->HeadPos;
            }

            bool IsValid() const {
                return P->Data[Pos].Generation == Gen;
            }

            TData& GetValue() const {
                Y_ASSERT(IsValid());
                return  P->Data[Pos].Value;
            }

            TIndexType GetKey() const {
                Y_ASSERT(IsValid());
                return Pos;
            }

            std::pair<TIndexType, TData> operator*() const {
                Y_ASSERT(IsValid());
                return {Pos, P->Data[Pos].Value};
            }

            bool operator!=(TEndIterTag) const {
                return IsValid();
            }

            void operator++() {
                Y_ASSERT(IsValid());
                Pos = P->Data[Pos].Next;
            }
        };

        TIter begin() const {
            Y_ASSERT(GenerationId > 0);
            return {*this};
        }

        TEndIterTag end() const {
            return {};
        }
        size_t size() const {
            return NumNonEmpties;
        }
        TIndexType Count() const {
            return NumNonEmpties;
        }

        void SaveToJson(NJson::TJsonValue& value) const {
            value["NumObjects"] = ::NModule::JsonValue(NumObjects);
            value["NumNonEmpties"] = ::NModule::JsonValue(NumNonEmpties);
            value["Head"] = ::NModule::JsonValue(HeadPos);
            value["GenerationId"] = ::NModule::JsonValue(GenerationId);
            //Not implemented: value["Data"] = ::NModule::JsonValue(Data);
        }

    private:

        size_t NumObjects = 0;
        TIndexType NumNonEmpties = 0;
        TIndexType HeadPos = 0;
        TGenerationType GenerationId = 0;
        TPoolPodHolder<TNode> Data;
    };
} // NCore
} // NTextMachine
