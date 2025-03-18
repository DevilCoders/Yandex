#pragma once

#include <util/generic/string.h>
#include <util/generic/strbuf.h>

namespace NAapi {
namespace NStore {

class TStatus {
private:
    enum class EStatus {
        ES_OK,
        ES_NOT_FOUND,
        ES_OVERFLOW,
        ES_FAILED
    };

    EStatus Status;
    TString Msg;

    TStatus(EStatus status, const TStringBuf& msg);

public:
    static TStatus Ok(const TStringBuf& msg = "");
    static TStatus NotFound(const TStringBuf& msg = "");
    static TStatus Overflow(const TStringBuf& msg = "");
    static TStatus Failed(const TStringBuf& msg = "");
    bool IsOk() const;
    bool IsNotFound() const;
    bool IsOverflow() const;
    bool IsFailed() const;

    TString Message() const;
};

}  // namespace NStore
}  // namespace NAapi
