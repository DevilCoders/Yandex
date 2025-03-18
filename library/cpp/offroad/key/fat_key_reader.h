#pragma once

#include <library/cpp/offroad/fat/fat_searcher.h>

#include "fat_key_seeker.h"

namespace NOffroad {
    template <class Serializer, class Base>
    class TFatKeyReader: public Base {
    public:
        using TKey = typename Base::TKey;
        using TKeyRef = typename Base::TKeyRef;
        using TKeyData = typename Base::TKeyData;

        TFatKeyReader() {
        }

        template <class... Args>
        TFatKeyReader(const TArrayRef<const char>& fat, const TArrayRef<const char>& fatsub, Args&&... args)
            : Base(std::forward<Args>(args)...)
            , Seeker_(fat, fatsub)
        {
        }

        template <class... Args>
        TFatKeyReader(const TBlob& fat, const TBlob& fatsub, Args&&... args)
            : Base(std::forward<Args>(args)...)
            , Seeker_(fat, fatsub)
        {
        }

        template <class... Args>
        void Reset(const TArrayRef<const char>& fat, const TArrayRef<const char>& fatsub, Args&&... args) {
            Base::Reset(std::forward<Args>(args)...);
            Seeker_.Reset(fat, fatsub);
        }

        template <class... Args>
        void Reset(const TBlob& fat, const TBlob& fatsub, Args&&... args) {
            Base::Reset(std::forward<Args>(args)...);
            Seeker_.Reset(fat, fatsub);
        }

        using Base::Seek;

        bool LowerBound(const TKeyRef& prefix) {
            TKeyRef key;
            TKeyData data;
            return Seeker_.LowerBound(prefix, &key, &data, static_cast<Base*>(this));
        }

    private:
        TFatKeySeeker<TKeyData, Serializer> Seeker_;
    };

}
