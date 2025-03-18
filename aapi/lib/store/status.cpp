#include "status.h"

namespace NAapi {
namespace NStore {

TStatus::TStatus(EStatus status, const TStringBuf& msg)
    : Status(status)
    , Msg(msg)
{
}

TStatus TStatus::Ok(const TStringBuf& msg) {
    return TStatus(EStatus::ES_OK, msg);
}

TStatus TStatus::NotFound(const TStringBuf& msg) {
    return TStatus(EStatus::ES_NOT_FOUND, msg);
}

TStatus TStatus::Overflow(const TStringBuf& msg) {
    return TStatus(EStatus::ES_OVERFLOW, msg);
}

TStatus TStatus::Failed(const TStringBuf& msg) {
    return TStatus(EStatus::ES_FAILED, msg);
}

bool TStatus::IsOk() const {
    return Status == EStatus::ES_OK;
}

bool TStatus::IsNotFound() const {
    return Status == EStatus::ES_NOT_FOUND;
}

bool TStatus::IsOverflow() const {
    return Status == EStatus::ES_OVERFLOW;
}

bool TStatus::IsFailed() const {
    return Status == EStatus::ES_NOT_FOUND;
}

TString TStatus::Message() const {
    return Msg;
}

}  // namespace NStore
}  // namespace NAapi
