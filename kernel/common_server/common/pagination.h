#pragma once

#include "processor.h"

#include <kernel/common_server/util/accessor.h>
#include <library/cpp/object_factory/object_factory.h>
#include <util/generic/ptr.h>
#include <util/generic/utility.h>

enum class EPaginationType {
    OffsetLimit /* offset_limit */
};

class IPaginationParams {
public:
    using TPtr = THolder<IPaginationParams>;
    using TFactory = NObjectFactory::TObjectFactory<IPaginationParams, EPaginationType>;

    virtual ~IPaginationParams() = default;

    static TPtr Construct(EPaginationType type) {
        return TPtr(IPaginationParams::TFactory::Construct(type));
    }

    Y_WARN_UNUSED_RESULT virtual bool DeserializeFromContext(const IRequestProcessor& processor) = 0;

    virtual EPaginationType GetType() const = 0;

    template <class T>
    const T* GetAs() const {
        return dynamic_cast<const T*>(this);
    }
};

class TPaginationOffsetLimitParams : public IPaginationParams {
public:
    static constexpr ui64 DefaultLimit = 20;
private:
    CSA_READONLY(ui64, Offset, 0);
    CSA_READONLY(ui64, Limit, DefaultLimit);
public:
    TPaginationOffsetLimitParams() = default;

    TPaginationOffsetLimitParams(ui64 offset, ui64 limit)
        : Offset(offset)
        , Limit(limit)
    {}

    Y_WARN_UNUSED_RESULT bool DeserializeFromContext(const IRequestProcessor& processor) override {
        Offset = processor.GetValue<ui64>(processor.GetContext()->GetCgiParameters(), "offset", false).GetOrElse(Offset);
        Limit = processor.GetValue<ui64>(processor.GetContext()->GetCgiParameters(), "limit", false).GetOrElse(Limit);
        return true;
    }

    static EPaginationType GetClass() {
        return EPaginationType::OffsetLimit;
    }

    EPaginationType GetType() const override {
        return GetClass();
    }

    void ClampLimit(ui64 maxLimit) {
        Limit = Min(Limit, maxLimit);
    }

private:
    static inline auto Registrator = TFactory::TRegistrator<TPaginationOffsetLimitParams>(GetClass());
};
