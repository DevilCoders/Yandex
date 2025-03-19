#pragma once

#include <kernel/doom/item_storage/proto/status.pb.h>
#include <kernel/doom/item_storage/proto/status_code.pb.h>

#include <util/generic/overloaded.h>

#include <util/generic/array_ref.h>
#include <util/generic/string.h>
#include <util/generic/variant.h>
#include <util/generic/vector.h>
#include <util/string/cast.h>

namespace NDoom::NItemStorage {

class TStatus {
public:
    static TStatus Ok();
    static TStatus Err(EStatusCode code, TString message = {}, TVector<TString> context = {});

public:
    bool IsOk() const;
    bool IsErr() const;
    EStatusCode Code() const;

    TStringBuf Message() const&;
    TString Message() &&;

    TConstArrayRef<TString> Context() const&;
    TVector<TString> Context() &&;

public:
    TStatus& WithContext(TString context) {
        Context_.push_back(std::move(context));
        return *this;
    }

    TStatus& WithContext(TVector<TString> context) {
        Context_.insert(Context_.end(), std::move_iterator{context.begin()}, std::move_iterator{context.end()});
        return *this;
    }

private:
    TStatus();
    TStatus(EStatusCode code, TString message, TVector<TString> context);

private:
    EStatusCode Code_;
    TString Message_;
    TVector<TString> Context_;
};

// Simple helpers
const TStatus& StatusOk();
TStatus StatusErr(EStatusCode code, TString message, TVector<TString> context = {});

class TStatusException : public yexception {
public:
    static TStatusException FromStatus(const TStatus& status) {
        return TStatusException{} << "Unhandled status: " << status;
    }
};

NProto::TStatus StatusToProto(const TStatus& status);
void StatusToProto(const TStatus& status, NProto::TStatus* dst);
TStatus StatusFromProto(const NProto::TStatus& status);

namespace NDetail {

struct TOkTag {};
struct TErrTag {};

} // namespace NDetail

template <typename T>
class TStatusOr {
    using TErrTag = NDetail::TErrTag;
    using TOkTag = NDetail::TOkTag;

    template <typename U>
    friend class TStatusOr;

    struct TStatusHolder {
        template <typename ...Args>
        TStatusHolder(Args&& ...args)
            : Status{std::forward<Args>(args)...}
        {
        }

        TStatus Status;
    };

public:
    TStatusOr(T value)
        : TStatusOr{TOkTag{}, std::move(value)}
    {}

    TStatusOr(TStatus err)
        : TStatusOr{TErrTag{}, std::move(err)}
    {}

public:
    bool IsOk() const {
        return std::holds_alternative<T>(Value_);
    }

    bool IsErr() const {
        return std::holds_alternative<TStatusHolder>(Value_);
    }

    // Construct value inplace
    template <typename ...Args>
    T& EmplaceOk(Args&& ...args) {
        return Value_.template emplace<T>(std::forward<Args>(args)...);
    }

    // Construct error inplace
    template <typename ...Args>
    TStatus& EmplaceErr(Args&& ...args) {
        return Value_.template emplace<TStatusHolder>(std::forward<Args>(args)...).Status;
    }

    // Visit value or error
    template <typename OnOk, typename OnError>
    decltype(auto) Visit(OnOk&& ok, OnError&& error) & {
        return VisitImpl<TStatusHolder&>(*this, std::forward<OnOk>(ok), std::forward<OnError>(error));
    }

    template <typename OnOk, typename OnError>
    decltype(auto) Visit(OnOk&& ok, OnError&& error) const& {
        return VisitImpl<TStatusHolder const&>(*this, std::forward<OnOk>(ok), std::forward<OnError>(error));
    }

    template <typename OnOk, typename OnError>
    decltype(auto) Visit(OnOk&& ok, OnError&& error) && {
        return VisitImpl<TStatusHolder&&>(std::move(*this), std::forward<OnOk>(ok), std::forward<OnError>(error));
    }

    // Map ok value
    template <typename F>
    TStatusOr<std::invoke_result_t<F, T&&>> Map(F&& f) && {
        return MapImpl<TStatusHolder&&, T&&>(std::move(*this), std::forward<F>(f));
    }

    template <typename F>
    TStatusOr<std::invoke_result_t<F, T&>> Map(F&& f) & {
        return MapImpl<TStatusHolder&, T&>(*this, std::forward<F>(f));
    }

    template <typename F>
    TStatusOr<std::invoke_result_t<F, const T&>> Map(F&& f) const& {
        return MapImpl<const TStatusHolder&, const T&>(*this, std::forward<F>(f));
    }

    // Get status or TStatus::Ok()
    const TStatus& Status() const {
        if (auto* ptr = std::get_if<TStatusHolder>(&Value_); ptr) {
            return ptr->Status;
        }
        return StatusOk();
    }

    EStatusCode StatusCode() const {
        return Status().Code();
    }

    // Try get value
    T* Value() {
        return std::get_if<T>(&Value_);
    }

    const T* Value() const {
        return std::get_if<T>(&Value_);
    }

    // Get value or throw TStatusError
    T& Unwrap() & {
        return UnwrapImpl<T&>(*this);
    }

    // Get value or throw TStatusError
    const T& Unwrap() const& {
        return UnwrapImpl<const T&>(*this);
    }

    // Get value or throw TStatusError
    T Unwrap() && {
        return UnwrapImpl<T&&>(std::move(*this));
    }

    // Common helpers
    T& operator*() & {
        return Unwrap();
    }

    const T& operator*() const& {
        return Unwrap();
    }

    T operator*() && {
        return std::move(*this).Unwrap();
    }

    T* operator->() {
        return &Unwrap();
    }

    const T* operator->() const {
        return &Unwrap();
    }

    operator bool() const {
        return IsOk();
    }

    bool operator!() const {
        return IsErr();
    }

private:
    template <typename ...Args>
    TStatusOr(TOkTag, Args&& ...args)
        : Value_{std::in_place_type_t<T>{}, std::forward<Args>(args)...}
    {
    }

    template <typename ...Args>
    TStatusOr(TErrTag, Args&& ...args)
        : Value_{std::in_place_type_t<TStatusHolder>{}, std::forward<Args>(args)...}
    {
    }

    template <typename ErrType, typename Self, typename OnOk, typename OnError>
    static decltype(auto) VisitImpl(Self&& self, OnOk&& ok, OnError&& error) {
        return std::visit(TOverloaded{
            [&]<typename V>(V&& value) -> decltype(auto) {
                return std::invoke(std::forward<OnOk>(ok), std::forward<V>(value));
            },
            [&](ErrType status) -> decltype(auto) {
                return std::invoke(std::forward<OnError>(error), status.Status);
            },
        }, std::forward<Self>(self).Value_);
    }

    template <typename ErrType, typename ValueType, typename Self, typename F>
    static decltype(auto) MapImpl(Self&& self, F&& f) {
        using TResult = TStatusOr<std::invoke_result_t<F, ValueType>>;
        return VisitImpl<ErrType>(std::forward<Self>(self), [&f]<typename V>(V&& value) -> TResult {
            return TResult{TOkTag{}, std::invoke(std::forward<F>(f), std::forward<V>(value))};
        }, [](auto&& err) -> TResult {
            return TResult{TErrTag{}, std::forward<decltype(err)>(err)};
        });
    }

    template <typename Ret, typename Self>
    static Ret UnwrapImpl(Self&& self) {
        return std::forward<Self>(self).Visit([] (auto&& value) -> Ret {
            return std::forward<decltype(value)>(value);
        }, [](const TStatus& status) -> Ret {
            throw TStatusException::FromStatus(status);
        });
    }

private:
    std::variant<T, TStatusHolder> Value_;
};

} // namespace NDoom::NItemStorage
