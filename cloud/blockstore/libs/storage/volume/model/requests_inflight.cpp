#include "requests_inflight.h"

#include <library/cpp/containers/intrusive_rb_tree/rb_tree.h>

#include <util/generic/hash.h>

namespace NCloud::NBlockStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

struct TRequestsInFlight::TImpl
{
    struct TCompareByEnd
    {
        using is_transparent = void;

        template <typename T1, typename T2>
        static bool Compare(const T1& lhs, const T2& rhs)
        {
            return GetEnd(lhs) < GetEnd(rhs);
        }
    };

    struct TRequestNode
        : public TRbTreeItem<TRequestNode, TCompareByEnd>
    {
        TBlockRange64 BlockRange;

        TRequestNode(TBlockRange64 blockRange)
            : BlockRange(blockRange)
        {}
    };

    static ui64 GetEnd(const TRequestNode& node)
    {
        return node.BlockRange.End;
    }

    static ui64 GetEnd(ui64 end)
    {
        return end;
    }

    using TRequests = TRbTree<TRequestNode, TCompareByEnd>;

    THashMap<ui64, TRequestNode> NodeByRequestId;
    TRequests Requests;
};

////////////////////////////////////////////////////////////////////////////////

TRequestsInFlight::TRequestsInFlight()
    : Impl(new TImpl())
{}

TRequestsInFlight::~TRequestsInFlight()
{}

bool TRequestsInFlight::TryAddRequest(ui64 requestId, TBlockRange64 blockRange)
{
    auto lo = TImpl::TRequests::TIterator(
        Impl->Requests.LowerBound(blockRange.Start)
    );
    if (lo != Impl->Requests.End() && lo->BlockRange.Start <= blockRange.End) {
        // intersects with some other inflight request
        return false;
    }

    auto [it, emplaced] = Impl->NodeByRequestId.emplace(requestId, blockRange);
    Y_VERIFY(emplaced);

    Impl->Requests.Insert(it->second);
    return true;
}

void TRequestsInFlight::RemoveRequest(ui64 requestId)
{
    auto it = Impl->NodeByRequestId.find(requestId);
    if (it != Impl->NodeByRequestId.end()) {
        Impl->Requests.Erase(it->second);
        Impl->NodeByRequestId.erase(it);
    }
}

size_t TRequestsInFlight::Size() const
{
    return Impl->NodeByRequestId.size();
}

}   // namespace NCloud::NBlockStore::NStorage
