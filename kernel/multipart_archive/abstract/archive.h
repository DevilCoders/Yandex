#pragma once

#include "position.h"

#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/logger/global/global.h>
#include <util/generic/array_ref.h>
#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>

namespace NRTYArchive {
    class IFat {
    public:
        class IIterator {
        public:
            using TPtr = THolder<IIterator>;
            virtual ~IIterator() {}
            virtual bool IsValid() const = 0;
            virtual size_t GetId() const = 0;
            virtual void Next() = 0;
            virtual TPosition GetPosition() const = 0;
        };

        virtual ~IFat() {}

        virtual TPosition Get(size_t id) const = 0;
        virtual TPosition Set(size_t id, TPosition position) = 0;
        virtual ui64 Size() const = 0;
        virtual void Clear(ui64 reserve) = 0;
        virtual typename IIterator::TPtr GetIterator() const = 0;
        virtual TVector<TPosition> GetSnapshot() const = 0;
    };

    class TSequentialIterator final : public IFat::IIterator {
    public:
        explicit TSequentialIterator(TArrayRef<const TPosition> positions)
            : Positions(positions)
        {}

        explicit TSequentialIterator(TVector<TPosition>&& positions)
            : PositionsStorage(std::move(positions))
            , Positions(PositionsStorage)
        {}

        bool IsValid() const override {
            return Index < Positions.size();
        }

        size_t GetId() const override {
            return Index;
        }

        void Next() override {
            ++Index;
        }

        TPosition GetPosition() const override {
            return Positions[Index];
        }

    private:
        TVector<TPosition> PositionsStorage;
        TArrayRef<const TPosition> Positions;
        size_t Index = 0;
    };

    TVector<size_t> GetIdsFromSnapshot(TArrayRef<const TPosition> snapshot);

    template<class T>
    class IDocDeserializer {
    public:
        virtual ~IDocDeserializer() {}
        virtual THolder<T> Deserialize(const TBlob& blob) const = 0;

        THolder<T> operator()(const TBlob& blob) {
            return blob.Length() ? Deserialize(blob) : nullptr;
        }
    };

    class IReadableArchive {
    public:
        virtual ~IReadableArchive() = default;
        virtual TBlob GetDocument(ui32 docid) const = 0;
    };
}
