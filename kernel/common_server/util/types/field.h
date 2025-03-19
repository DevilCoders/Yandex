#pragma once

#include <util/generic/strbuf.h>

#include <tuple>

template <class T>
struct TField {
public:
    TField(T&& value, TStringBuf name, i64 index)
        : Value(std::forward<T>(value))
        , Name(name)
        , Index(index)
    {
    }

    template <class TT>
    bool operator==(const TField<TT>& other) const {
        return Tuple() == other.Tuple();
    }

    TStringBuf GetName() const {
        return Name;
    }
    bool HasName() const {
        return !Name.empty();
    }

    ui64 GetIndex() const {
        Y_ASSERT(HasIndex());
        return Index;
    }
    bool HasIndex() const {
        return Index >= 0;
    }

    auto Tuple() const {
        return std::tie(Value, Name);
    }

public:
    T&& Value;

private:
    TStringBuf Name;
    i64 Index;
};

template <class T>
auto Field(T&& value, TStringBuf name, i64 index = -1) {
    return TField<T>(std::forward<T>(value), name, index);
}

#define DECLARE_FIELDS(...)                     \
    auto GetFields() const {                    \
        return std::make_tuple(__VA_ARGS__);    \
    }                                           \
    auto GetFields() {                          \
        return std::make_tuple(__VA_ARGS__);    \
    }
