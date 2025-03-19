#include "addr.h"
#include "shard.h"

void IAddrDelivery::OnEvent(const NNeh::IMultiClient::TEvent& ev) {
    THolder<IAddrDelivery> this_(this);
    Owner->AddResponse(ev, Now() - StartTime, AttIdx);
}

IAddrDelivery::IAddrDelivery(IShardDelivery* owner, const ui32 attIdx)
    : TGuard<IShardDelivery>(*owner)
    , Owner(owner)
    , AttIdx(attIdx)
    , StartTime(Now())
{
}

IAddrDelivery::~IAddrDelivery() {
}
