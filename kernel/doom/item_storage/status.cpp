#include "status.h"

#include <library/cpp/iterator/enumerate.h>

namespace NDoom::NItemStorage {

static inline const TStatus OK_STATUS = TStatus::Ok();

TStatus TStatus::Ok() {
    return TStatus{};
}

TStatus TStatus::Err(EStatusCode code, TString message, TVector<TString> context) {
    return TStatus{code, std::move(message), std::move(context)};
}

const TStatus& StatusOk() {
    return OK_STATUS;
}

TStatus StatusErr(EStatusCode code, TString message, TVector<TString> context) {
    return TStatus::Err(code, std::move(message), std::move(context));
}

bool TStatus::IsOk() const {
    return Code_ == EStatusCode::Ok;
}

bool TStatus::IsErr() const {
    return !IsOk();
}

EStatusCode TStatus::Code() const {
    return Code_;
}

TStringBuf TStatus::Message() const& {
    return Message_;
}

TString TStatus::Message() && {
    return std::move(Message_);
}

TConstArrayRef<TString> TStatus::Context() const& {
    return Context_;
}

TVector<TString> TStatus::Context() && {
    return std::move(Context_);
}

TStatus::TStatus()
    : TStatus{EStatusCode::Ok, {}, {}}
{
}

TStatus::TStatus(EStatusCode code, TString message, TVector<TString> context)
    : Code_{code}
    , Message_{std::move(message)}
    , Context_{std::move(context)}
{
}

NProto::TStatus StatusToProto(const TStatus& status) {
    NProto::TStatus res;
    StatusToProto(status, &res);
    return res;
}

void StatusToProto(const TStatus& status, NProto::TStatus* dst) {
    dst->SetCode(status.Code());
    dst->SetMessage(status.Message().data(), status.Message().size());
    dst->MutableContext()->Reserve(status.Context().size());
    for (TStringBuf ctx : status.Context()) {
        dst->AddContext(ctx.data(), ctx.size());
    }
}

TStatus StatusFromProto(const NProto::TStatus& status) {
    EStatusCode code = status.GetCode();
    if (code == EStatusCode::DoNotUseReservedForFutureExpansion) {
        code = EStatusCode::Unknown;
    }
    return StatusErr(code, status.GetMessage(), TVector<TString>{status.GetContext().begin(), status.GetContext().end()});
}

} // namespace NDoom::NItemStorage

template <>
void Out<::NDoom::NItemStorage::TStatus>(IOutputStream& out, const ::NDoom::NItemStorage::TStatus& status) {
    out << "{Code: " << status.Code();
    if (status.Message()) {
        out << "; Message: " << status.Message();
    }
    if (auto context = status.Context()) {
        out << "; Context: [";
        for (auto&& [i, line] : Enumerate(context)) {
            if (i > 0) {
                out << ',';
            }
            out << "\"" << line << "\"";
        }
        out << ']';
    }
    out << '}';
}
