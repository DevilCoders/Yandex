#pragma once

#include <library/cpp/json/json_value.h>

namespace NJson {
    class TBaseBuilder {
    public:
        inline operator TJsonValue() {
            return std::move(Object);
        }

        const NJson::TJsonValue& GetJson() const {
            return Object;
        }

    protected:
        TJsonValue Object;
    };

    class TArrayBuilder: public TBaseBuilder {
    public:
        template <class... TArgs>
        TArrayBuilder(TArgs&&... args) {
            operator()(std::forward<TArgs>(args)...);
        }
        TArrayBuilder& operator() (const TJsonValue& value) {
            Object.AppendValue(value);
            return *this;
        }
        TArrayBuilder& operator() (TJsonValue&& value) {
            Object.AppendValue(std::move(value));
            return *this;
        }
    };

    class TMapBuilder: public TBaseBuilder {
    public:
        template <class... TArgs>
        TMapBuilder(TArgs&&... args) {
            operator()(std::forward<TArgs>(args)...);
        }
        TMapBuilder& operator() (TStringBuf key, const TJsonValue& value) {
            Object[key] = value;
            return *this;
        }
        TMapBuilder& operator() (TStringBuf key, TJsonValue&& value) {
            Object[key] = std::move(value);
            return *this;
        }
    };
}
