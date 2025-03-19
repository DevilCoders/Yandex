#include "filter.h"

namespace NCommonProxy {
    class TFilter::TLinkFilter final : public TLink::IFilter{
    public:
        TLinkFilter(const TFilter& owner)
            : Owner(owner)
        {}

        virtual bool Accepted(const TDataSet::TPtr input) const override {
            return input && Owner.Accepted(input);
        }

    private:
        const TFilter& Owner;
    };

    const TMetaData& NCommonProxy::TFilter::GetOutputMetaData() const {
        return GetInputMetaData();
    }

    void TFilter::AddRequester(TLink::TPtr link) {
        TConverter::AddRequester(link);
        link->SetFilter<TLinkFilter>(*this);
    }

    bool TFilter::Convert(TDataSet::TPtr input, TDataSet::TPtr& output, IReplier::TPtr /*replier*/) const {
        output = input;
        return true;
    }
}
