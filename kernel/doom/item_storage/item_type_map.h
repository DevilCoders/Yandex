#pragma once

#include "types.h"

#include <library/cpp/containers/stack_vector/stack_vec.h>

#include <util/generic/array_ref.h>

namespace NDoom::NItemStorage {

template <typename T>
class TItemTypeMap {
    inline static constexpr ui32 EmptyValue = Max<ui32>();

public:
    struct TEmplaceResult {
        T& Value;
        bool Emplaced = false;
    };

public:
    T& operator[](TItemType itemType) {
        auto&& [result, emplaced] = EmplaceImpl(itemType);
        return result;
    }

    T* FindPtr(TItemType type) {
        return FindPtrImpl(*this, type);
    }

    const T* FindPtr(TItemType type) const {
        return FindPtrImpl(*this, type);
    }

    T& at(TItemType type) {
        if (T* ptr = FindPtr(type)) {
            return *ptr;
        }
        throw std::out_of_range{"Not found"};
    }

    const T& at(TItemType type) const {
        if (const T* ptr = FindPtr(type)) {
            return *ptr;
        }
        throw std::out_of_range{"Not found"};
    }

    TEmplaceResult insert(TItemType itemType, T&& value) {
        return emplace(itemType, std::move(value));
    }

    template <typename ...Args>
    TEmplaceResult emplace(TItemType itemType, Args&& ...args) {
        return EmplaceImpl(itemType, std::forward<Args>(args)...);
    }

    bool HasItemType(TItemType type) const {
        return Remapping_.size() > type && Remapping_[type] != EmptyValue;
    }

    size_t Size() const {
        return ItemTypes_.size();
    }

    bool Empty() const {
        return ItemTypes_.empty();
    }

    TConstArrayRef<TItemType> ItemTypes() const {
        return ItemTypes_;
    }

    TConstArrayRef<T> Values() const {
        return Values_;
    }

    // FIXME(sskvor): Add iterator

private:
    template <typename ...Args>
    TEmplaceResult EmplaceImpl(TItemType type, Args&& ...args) {
        ResizeToFit(type);

        bool emplaced = false;
        if (Remapping_[type] == EmptyValue) {
            Remapping_[type] = Values_.size();
            Values_.emplace_back(std::forward<Args>(args)...);
            ItemTypes_.push_back(type);
            emplaced = true;
        }

        return { Values_[Remapping_[type]], emplaced };
    }

    void ResizeToFit(TItemType itemType) {
        if (Remapping_.size() <= itemType) {
            Remapping_.resize(itemType + 1, EmptyValue);
        }
    }

    template <typename Self>
    static auto FindPtrImpl(Self& self, TItemType type) {
        using PointerType = decltype(&self.Values_[self.Remapping_[type]]);
        if (self.HasItemType(type)) {
            return &self.Values_[self.Remapping_[type]];
        }
        return static_cast<PointerType>(nullptr);
    }

private:
    TSmallVec<ui32> Remapping_;
    TStackVec<T, 4> Values_;
    TStackVec<TItemType, 4> ItemTypes_;
};

} // namespace NDoom::NItemStorage
