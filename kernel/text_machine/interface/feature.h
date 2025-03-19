#pragma once

#include "feature_traits.h"
#include "feature_part.h"

#include <kernel/text_machine/structured_id/full_id.h>

#include <util/generic/hash.h>

namespace NTextMachine {
    namespace NFeatureInternals {
        using TFullFeatureIdBase = ::NStructuredId::TFullId<EFeaturePartType>;
    } // NFeatureInternals

    class TFullFeatureId
        : public NFeatureInternals::TFullFeatureIdBase
    {
    public:
        TFullFeatureId() = default;
        TFullFeatureId(const TFullFeatureId& other)
            : NFeatureInternals::TFullFeatureIdBase(static_cast<const NFeatureInternals::TFullFeatureIdBase&>(other))
        {}

        template <typename T, typename... Args>
        Y_FORCE_INLINE TFullFeatureId(const T& arg1, const Args& ... args) {
            Set(arg1, args...);
        }

        TFullFeatureId& operator = (const TFullFeatureId&) = default;
        using NFeatureInternals::TFullFeatureIdBase::operator ==;
        using NFeatureInternals::TFullFeatureIdBase::operator <;
        using NFeatureInternals::TFullFeatureIdBase::Hash;

        template <typename ArgX, typename ArgY, typename... Args>
        Y_FORCE_INLINE void Set(const ArgX& argX, const ArgY& argY, const Args& ... args) {
            Set(argX);
            Set(argY, args...);
        }
        template <typename T>
        Y_FORCE_INLINE void Set(T value) {
            NFeatureInternals::TFullFeatureIdBase::Set(value);
        }
        Y_FORCE_INLINE void Set(const TFullFeatureId& id) {
            (*this) = id;
        }

        // Part-specific helpers
        //
        Y_FORCE_INLINE void Set(EStreamType value) {
            Set(TStreamSet(value));
        }
        Y_FORCE_INLINE void Set(EStreamSetType value) {
            Set(TStreamSet(value));
        }
        Y_FORCE_INLINE void Set(const char* name) {
            Set(TStringBuf(name));
        }

     };

    using TFullFeatureIds = TVector<TFullFeatureId>;
    using TFFId = TFullFeatureId;
    using TFFIds = TFullFeatureIds;

    class TFFIdWithHash
        : private TFFId
    {
        static const ui64 EmptyHash;
        ui64 SavedHash = EmptyHash;

    public:
        TFFIdWithHash() = default;
        TFFIdWithHash(const TFFIdWithHash&) = default;
        TFFIdWithHash(const TFFId& id)
            : TFFId(id)
            , SavedHash(id.Hash())
        {}

        TFFIdWithHash& operator = (const TFFIdWithHash& other) = default;
        TFFIdWithHash& operator = (const TFFId& other) {
            TFFId::operator == (other);
            SavedHash = other.Hash();
            return *this;
        }

        bool operator == (const TFFIdWithHash& other) const {
            return TFFId::operator == (other);
        }
        bool operator < (const TFFIdWithHash& other) const {
            return TFFId::operator < (other);
        }

        using TFFId::Get;
        using TFFId::IsValid;
        using TFFId::FullName;

        ui64 Hash() const {
            return SavedHash;
        }
        const TFFId& ToId() const {
            return *this;
        }
    };

    class TFeature {
    public:
        TFFId Id;
        float Value = 0.0f;

    public:
        TFeature() = default;
        TFeature(float value)
            : Value(value)
        {}
        TFeature(const TFFId& id, float value)
            : Id(id), Value(value)
        {}

        void SetId(const TFFId& id) {
            Id = id;
        }
        void SetValue(float value) {
            Value = value;
        }

        const TFFId& GetId() const {
            return Id;
        }
        float GetValue() const {
            return Value;
        }
    };

    class TOptFeature {
        const TFFIdWithHash* Id = nullptr;
        float Value = 0.0f;

    public:
        TOptFeature() = default;
        explicit TOptFeature(float value)
            : Value(value)
        {}
        TOptFeature(const TFFIdWithHash* id, float value)
            : Id(id)
            , Value(value)
        {}

        void SetIdWithHash(const TFFIdWithHash* id) {
            Id = id;
        }
        void SetValue(float value) {
            Value = value;
        }

        const TFFIdWithHash& GetIdWithHash() const {
            static const TFFIdWithHash emptyId{};
            return (Id ? *Id : emptyId);
        }
        const TFFId& GetId() const {
            return GetIdWithHash().ToId();
        }
        float GetValue() const {
            return Value;
        }

        operator TFeature () const {
            return {GetId(), GetValue()};
        }
    };

    using TFeatures = TVector<TFeature>;
    using TOptFeatures = TVector<TOptFeature>;
}

template <>
struct THash<NTextMachine::TFullFeatureId> {
    ui64 operator() (const NTextMachine::TFullFeatureId& x) {
        return x.Hash();
    }
};

template <>
struct THash<NTextMachine::TFFIdWithHash> {
    ui64 operator() (const NTextMachine::TFFIdWithHash& x) const {
        return x.Hash();
    }
};

