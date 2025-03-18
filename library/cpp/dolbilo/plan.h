#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/ylimits.h>

#include <util/datetime/base.h>

#include <library/cpp/http/io/headers.h>

class IInputStream;
class IOutputStream;

// Generic description of an HTTP request
struct TDevastateRequest {
    // Full url
    TString Url;

    // If not present, "Host" will be set automatically based on the url
    THttpHeaders Headers;

    // Allowed to contain arbitrary binary data
    TString Body;

    // Http method, in capital letters
    // If left empty, assumed to be "GET" for requests with empty body, otherwise "POST"
    TString Method;
};

class TDevastateItem {
public:
    TDevastateItem(const TString& url, const TDuration& toWait, const TString& headers, ui64 planIndex);
    TDevastateItem(const TDuration& toWait, const TDevastateRequest& requestDescription, ui64 planIndex);

    // Data is supposed to be a complete HTTP/1.1 request message with
    // request target in the origin form ("/path[?query]") and a valid "Host" header
    TDevastateItem(const TDuration& toWait, const TString& host, ui16 port, const TString& data, ui64 planIndex);

    inline ~TDevastateItem() = default;

    inline ui64 PlanIndex() const noexcept {
        return PlanIndex_;
    }

    inline ui16 Port() const noexcept {
        return Port_;
    }

    inline const TString& Host() const noexcept {
        return Host_;
    }

    inline const TString& Data() const noexcept {
        return Data_;
    }

    inline const TDuration& ToWait() const noexcept {
        return ToWait_;
    }

    inline void ToWait(const TDuration& val) noexcept {
        ToWait_ = val;
    }

    TString GenerateUrl() const;

    static TDevastateItem Load(IInputStream* stream, ui64 planIndex);
    void Save(IOutputStream* stream) const;

private:
    TString Host_;
    TString Data_;
    TDuration ToWait_;
    ui16 Port_;
    ui64 PlanIndex_ = Max<ui64>();
};

class TDevastatePlan {
    typedef TVector<TDevastateItem> TStorage;

public:
    typedef TStorage::const_iterator const_iterator;

    inline TDevastatePlan() {
    }

    TDevastatePlan(IInputStream* stream);

    inline const_iterator Begin() const noexcept {
        return Storage_.begin();
    }

    inline const_iterator End() const noexcept {
        return Storage_.end();
    }

    inline size_t Size() const noexcept {
        return Storage_.size();
    }

    inline bool Empty() const noexcept {
        return Storage_.empty();
    }

    inline void Add(const TDevastateItem& item) {
        Storage_.push_back(item);
    }

    inline void Clear() noexcept {
        Storage_.clear();
    }

    void Save(IOutputStream* stream) const;

private:
    TStorage Storage_;
};

enum EVerdict {
    V_CONTINUE,
    V_BREAK,
};

class IDevastatePlanFunctor {
public:
    inline IDevastatePlanFunctor() noexcept = default;

    virtual ~IDevastatePlanFunctor() = default;

    virtual EVerdict Process(const TDevastateItem& item) = 0;
};

void ForEachPlanItem(IInputStream* in, IDevastatePlanFunctor& func, i64 limit, size_t timeLimit);

template <class TFunctor>
static inline void ForEachPlanItem(IInputStream* in, TFunctor& func, i64 limit = Max<i64>(), size_t timeLimit = 0) {
    class TDevastatePlanFunctor: public IDevastatePlanFunctor {
    public:
        inline TDevastatePlanFunctor(TFunctor& f) noexcept
            : F_(f)
        {
        }

        ~TDevastatePlanFunctor() override = default;

        EVerdict Process(const TDevastateItem& item) override {
            return F_(item);
        }

    private:
        TFunctor& F_;
    };

    TDevastatePlanFunctor binder(func);

    ForEachPlanItem(in, (IDevastatePlanFunctor&)binder, limit, timeLimit);
}
