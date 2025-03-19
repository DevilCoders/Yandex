#include "incomplete_requests.h"

namespace NCloud {

namespace {

////////////////////////////////////////////////////////////////////////////////

class TIncompleteRequestProvider
    : public IIncompleteRequestProvider
{
public:
    TIncompleteRequestProvider() = default;

    TVector<TIncompleteRequest> GetIncompleteRequests() override
    {
        return {};
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IIncompleteRequestProviderPtr CreateIncompleteRequestProviderStub()
{
    return std::make_shared<TIncompleteRequestProvider>();
}

}   // namespace NCloud
