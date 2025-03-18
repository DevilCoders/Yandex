#include <util/string/cast.h>
#include <util/memory/tempbuf.h>
#include <util/ysaveload.h>
#include <util/system/error.h>
#include <library/cpp/http/misc/httpcodes.h>

#include "stat.h"

static TString GetReqid(const TString& url) {
    size_t posStart = url.find("&reqid=");
    if (posStart != TString::npos) {
        posStart += 7; // cut "&reqid="
        size_t posEnd = url.find('&', posStart + 1);
        size_t reqidLen = posEnd;
        if (posEnd != TString::npos) {
            reqidLen = posEnd - posStart;
        }
        return url.substr(posStart, reqidLen);
    }

    return TString();
}

TDevastateStatItem::TDevastateStatItem(IInputStream* s) {
    try {
        ::Load(s, ErrCode_);
        ::Load(s, HttpCode_);
        ::Load(s, Url_);
        ::Load(s, TimeStats_.Start);
        ::Load(s, TimeStats_.End);
        ::Load(s, DataLength_);
        ::Load(s, Data_);
        ::Load(s, CustomErrDescr_);
    } catch (...) {
        /*
         * TODO
         */

        throw TCanNotLoad();
    }
}

void TDevastateStatItem::Save(IOutputStream* s) const {
    ::Save(s, ErrCode_);
    ::Save(s, HttpCode_);
    ::Save(s, Url_);
    ::Save(s, TimeStats_.Start);
    ::Save(s, TimeStats_.End);
    ::Save(s, DataLength_);
    ::Save(s, Data_);
    ::Save(s, CustomErrDescr_);
}

void TDevastateStatItem::DumpPhantom(IOutputStream* s, bool reqidAsAmmo) const {
    char buf[0xFF];
    snprintf(buf, sizeof(buf), "%.03f", TimeStats_.Start.SecondsFloat());

    *s << buf << "\t";

    if (reqidAsAmmo) {
        *s << '#';
    };

    *s << GetReqid(Url_) << "\t" // see SEARCH-1812
       << (TimeStats_.End - TimeStats_.Start).MicroSeconds() << "\t"
       << (TimeStats_.Handshake - TimeStats_.Start).MicroSeconds() << "\t"
       << (TimeStats_.Send - TimeStats_.Handshake).MicroSeconds() << "\t"
       << (TimeStats_.Response - TimeStats_.Send).MicroSeconds() << "\t"
       << (TimeStats_.End - TimeStats_.Response).MicroSeconds() << "\t"
       << (TimeStats_.End - TimeStats_.Start).MicroSeconds() << "\t"
       << RequestLength_ << "\t"
       << DataLength_ << "\t"
       << ErrCode_ << "\t"
       << HttpCode_
       << Endl;
}

TString TDevastateStatItem::ErrDescription() const {
    if (!CustomErrDescr_.empty()) {
        return CustomErrDescr_;
    }

    if (HttpCode_) {
        return TString(HttpCodeStr(HttpCode_));
    }

    switch (ErrCode_) {
        case TDevastateStatItem::PROTOCOL_ERROR:
            return "Protocol error";

        default:
            break;
    }

    return LastSystemErrorText(ErrCode_);
}
