#pragma once

#include "cast.h"
#include "type_traits.h"

#include <util/generic/string.h>
#include <util/memory/blob.h>

namespace NMms {
    //! Class for holding mmapped file together with object mapped to it.
    template <class Object>
    class TMapping {
    public:
        using TMmappedObject = typename TMmappedType<Object>::Type;
        template <class>
        friend class TMapping;

    public:
        TMapping() {
            Reset();
        }

        explicit TMapping(const TString& fileName, bool checkVersion = true) {
            Reset(fileName, checkVersion);
        }

        explicit TMapping(TBlob data, bool checkVersion = true) {
            Reset(std::move(data), checkVersion);
        }

        template <typename T>
        TMapping(const TMapping<T>& other) {
            Blob = other.Blob;
            Start = other.Start;
        }

        void Swap(TMapping<Object>& other) noexcept {
            DoSwap(Blob, other.Blob);
            DoSwap(Start, other.Start);
        }

        template <typename T>
        TMapping& operator=(const TMapping<T>& other) {
            TMapping<Object> tmp(other);
            Swap(tmp);
            return *this;
        }

        void Reset() noexcept {
            Blob = TBlob();
            Start = nullptr;
        }

        void Reset(const TString& fileName, bool checkVersion = true) {
            Reset(TBlob::PrechargedFromFile(fileName), checkVersion);
        }

        void Reset(TBlob data, bool checkVersion = true) {
            Blob = std::move(data);
            if (checkVersion) {
                Start = &SafeCast<TMmappedObject>(Blob.AsCharPtr(), Blob.Length());
            } else {
                Start = &UnsafeCast<TMmappedObject>(Blob.AsCharPtr(), Blob.Length());
            }
        }

        explicit operator bool() const noexcept {
            return (Start != nullptr);
        }

        const TMmappedObject* Get() const {
            return Start;
        }

        const TMmappedObject* operator->() const {
            Y_ASSERT(*this);
            return Get();
        }

        const TMmappedObject& operator*() const {
            Y_ASSERT(*this);
            return *(Get());
        }

    private:
        TBlob Blob;
        const TMmappedObject* Start; // holds object start address in BLOB (useful for casted object manipulation)
    };
} //namespace NMms
