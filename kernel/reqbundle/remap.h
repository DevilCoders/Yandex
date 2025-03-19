#pragma once

#include "reqbundle_fwd.h"
#include "facets.h"

#include <library/cpp/json/json_value.h>

#include <util/generic/list.h>
#include <util/generic/explicit_type.h>

namespace NReqBundle {
    class IValueRemap
        : public TAtomicRefCount<IValueRemap>
    {
    public:
        virtual ~IValueRemap () {}

        virtual float Ku(float value) = 0;
        virtual NJson::TJsonValue ToJson() const = 0;
    };

    using TValueRemapPtr = TIntrusivePtr<IValueRemap>;

    class TConstValRemap
        : public IValueRemap
    {
    public:
        float Value = 1.0f;

        ~TConstValRemap() override {}

        float Ku(float /*value*/) override {
            return Value;
        }

        static THolder<TConstValRemap> TryFromJson(
            const TExplicitType<NJson::TJsonValue>& value
        );

        NJson::TJsonValue ToJson() const override;
    };

    class TQuantizeRemap
        : public IValueRemap
    {
    public:
        size_t N = 255;

        ~TQuantizeRemap() override {}

        float Ku(float value) override {
            return static_cast<size_t>(float(N) * value + 0.5f) / float(N);
        }

        static THolder<TQuantizeRemap> TryFromJson(
            const TExplicitType<NJson::TJsonValue>& value
        );

        NJson::TJsonValue ToJson() const override;
    };

    class TClipRemap
        : public IValueRemap
    {
    public:
        float MinValue = 0.0f;
        float MaxValue = 1.0f;

        ~TClipRemap() override {}

        float Ku(float value) override {
            return Max(MinValue, Min(MaxValue, value));
        }

        static THolder<TClipRemap> TryFromJson(
            const TExplicitType<NJson::TJsonValue>& value
        );

        NJson::TJsonValue ToJson() const override;
    };

    class TRenormFracRemap
        : public IValueRemap
    {
    public:
        float Scale = 1.0f;
        float Center = 0.0f;
        float Offset = 0.0f;

        ~TRenormFracRemap() override {}

        float Ku(float value) override {
            return Scale * ((value + Offset) / (value + Center + 2 * Offset));
        }

        static THolder<TRenormFracRemap> TryFromJson(
            const TExplicitType<NJson::TJsonValue>& value
        );

        NJson::TJsonValue ToJson() const override;
    };

    class TFacetRemap {
    public:
        TFacetId Facet;
        TValueRemapPtr Remap;

        TFacetRemap() = default;
        TFacetRemap(const TFacetId& facet, const TValueRemapPtr& remap)
            : Facet(facet)
            , Remap(remap)
        {}

        void FromJson(const TExplicitType<NJson::TJsonValue>& value);
        NJson::TJsonValue ToJson() const;
    };

    class TAllRemapOptions
    {
    public:
        using TData = TList<TFacetRemap>;
        using TConstIterator = typename TData::const_iterator;
        using TIterator = typename TData::iterator;

        TAllRemapOptions() = default;

        template <typename... Args>
        TAllRemapOptions(Args&&... args)
        {
            Add(std::forward<Args>(args)...);
        }

        TFacetRemap& Add() {
            Data.emplace_back();
            return Data.back();
        }

        TDeque<TValueRemapPtr> FindAll(const TFacetId& id) const;
        TDeque<TFacetRemap> FindAllApplicable(const TFacetId& id) const;

        TAllRemapOptions& FromJson(const TExplicitType<NJson::TJsonValue>& value);
        TAllRemapOptions& FromJsonString(TStringBuf text);

        NJson::TJsonValue ToJson() const;

        TConstIterator begin() const {
            return Data.begin();
        }

        TIterator begin() {
            return Data.begin();
        }

        TConstIterator end() const {
            return Data.end();
        }

        TIterator end() {
            return Data.end();
        }

        size_t size() const {
            return Data.size();
        }

        bool empty() const {
            return Data.empty();
        }

        void clear() {
            Data.clear();
        }

    private:
        TData Data;
    };

    void RemapValues(
        TRequestAcc request,
        const TAllRemapOptions& options);

    void RemapValues(
        TReqBundleAcc bundle,
        const TAllRemapOptions& options);
} // NReqBundle
