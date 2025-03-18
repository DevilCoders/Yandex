#pragma once

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

class IInputStream;
class IOutputStream;

class TDevastateStatItem {
public:
    static const unsigned CUSTOM_ERR_CODE = 1000000;
    static const unsigned PROTOCOL_ERROR = CUSTOM_ERR_CODE + 1;

    using TData = TVector<unsigned char>;

    /** Time statistics structure. */
    struct TTimeStatItem {
        TInstant Start;     // Start moment.
        TInstant Handshake; // Handshake processed moment.
        TInstant Send;      // Request fully send moment.
        TInstant Response;  // First response data received moment.
        TInstant End;       // Response fully received moment.
    };

    class TCanNotLoad {};

    TDevastateStatItem(IInputStream* s);

    inline TDevastateStatItem(const TString& url, const TTimeStatItem& tstats, ui64 planIndex, unsigned errcode, const TString& errdescr = TString())
        : ErrCode_(errcode)
        , Url_(url)
        , TimeStats_(tstats)
        , PlanIndex_(planIndex)
        , CustomErrDescr_(errdescr)
    {
        Y_ASSERT(planIndex != Max<ui64>() && "Response index not set");
        TimeStats_.Send = TimeStats_.Handshake = TimeStats_.Start;
        TimeStats_.Response = TimeStats_.End = Now();
    }

    inline TDevastateStatItem(const TString& url, const TTimeStatItem& tstats, ui64 planIndex, size_t reqlen, unsigned httpcode, TData& data, size_t datalen)
        : HttpCode_(httpcode)
        , Url_(url)
        , TimeStats_(tstats)
        , PlanIndex_(planIndex)
        , RequestLength_(reqlen)
        , DataLength_(datalen)
    {
        Y_VERIFY(planIndex != std::numeric_limits<ui64>::max() && "Response index not set");
        Data_.swap(data);
        TimeStats_.End = Now();
    }

    inline ~TDevastateStatItem() = default;

    inline const TString& Url() const noexcept {
        return Url_;
    }

    inline const TTimeStatItem& TimeStats() const noexcept {
        return TimeStats_;
    }

    inline const TInstant& StartTime() const noexcept {
        return TimeStats_.Start;
    }

    inline const TInstant& EndTime() const noexcept {
        return TimeStats_.End;
    }

    inline unsigned HttpCode() const noexcept {
        return HttpCode_;
    }

    inline unsigned ErrCode() const noexcept {
        return ErrCode_;
    }

    inline size_t RequestLength() const noexcept {
        return RequestLength_;
    }

    inline size_t DataLength() const noexcept {
        return DataLength_;
    }

    inline bool DataSaved() const noexcept {
        return DataLength_ == Data_.size();
    }

    inline const TData& Data() const noexcept {
        return Data_;
    }

    inline ui64 PlanIndex() const noexcept {
        return PlanIndex_;
    }

    TString ErrDescription() const;

    void Save(IOutputStream* s) const;

    void DumpPhantom(IOutputStream* s, bool reqidAsAmmo) const;

private:
    ui32 ErrCode_ = 0;
    ui32 HttpCode_ = 0;
    TString Url_;
    TTimeStatItem TimeStats_;
    ui64 PlanIndex_ = Max<ui64>();
    ui64 RequestLength_ = 0;
    TData Data_;
    ui64 DataLength_ = 0;
    TString CustomErrDescr_;
};

template <class F>
static inline void ForEachStatItem(IInputStream* s, F& f) {
    try {
        while (true) {
            TDevastateStatItem item(s);

            f(item);
        }
    } catch (const TDevastateStatItem::TCanNotLoad&) {
    }
}
