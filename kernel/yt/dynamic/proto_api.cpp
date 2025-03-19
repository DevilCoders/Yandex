#include "proto_api.h"

#include <yt/yt/core/misc/range.h>
#include <yt/yt/client/table_client/row_buffer.h>

#include <util/generic/hash.h>

namespace NYT {
namespace NProtoApi {

using namespace NTableClient;

TRowsetBuilderBase::TRowsetBuilderBase(TProtoSchemaPtr schema)
    : Schema(schema)
    , Buffer(New<TRowBuffer>())
{
}
TRowsetBuilderBase::~TRowsetBuilderBase() noexcept = default;

TSharedRange<TUnversionedRow> TRowsetBuilderBase::Finish() && {
    Rows.shrink_to_fit();
    TRowBufferPtr buffer = New<TRowBuffer>();
    buffer.Swap(Buffer);
    return MakeSharedRange(std::move(Rows), buffer);
}

TSharedRange<TUnversionedRow> TRowsetBuilderBase::Finish() const& {
    return MakeSharedRange(Rows, Buffer);
}

void TRowsetBuilderBase::Clear() {
    Buffer->Clear();
    Rows.clear();
}

} // NProtoApi

void WaitAndLock(
    NYT::NApi::ITransactionPtr transaction,
    const TString& nodePath,
    NYT::NCypressClient::ELockMode lockMode,
    TDuration timeout,
    NYT::NApi::TLockNodeOptions lockOptions)
{
    NLogging::TLogger Logger{NProtoApi::Logger};
    Y_ENSURE(transaction);
    lockOptions.Waitable = true;
    NYT::NCypressClient::TLockId lockId = NYT::NConcurrency::WaitFor(transaction->LockNode(
        nodePath,
        lockMode,
        lockOptions)).ValueOrThrow().LockId;
    TInstant start = TInstant::Now();

    TString lockFullPath = nodePath;
    if (lockOptions.ChildKey) {
        lockFullPath = NJupiter::JoinYtPath(lockFullPath, lockOptions.ChildKey.value());
    }
    if (lockOptions.AttributeKey) {
        lockFullPath = NJupiter::JoinYtMeta(lockFullPath, lockOptions.AttributeKey.value());
    }

    while ((timeout.GetValue() == 0) || ((TInstant::Now() - start) < timeout)) {
        TString lockStateNodePath = "#" + ToString(lockId);
        if (GetYtAttr<TString>(transaction, lockStateNodePath, "state") == "acquired") {
            YT_LOG_INFO("%Qv lock for node %Qv has just been acquired.", lockMode, lockFullPath);
            return;
        }
        Sleep(TDuration::Seconds(2));
    }

    ythrow yexception() << " timed out waiting lock for " << lockFullPath;
}

} // NYT
