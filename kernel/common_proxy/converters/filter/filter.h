#pragma once

#include <kernel/common_proxy/common/converter.h>

namespace NCommonProxy {

    class TFilter : public TConverter {
    public:
        using TConverter::TConverter;
        virtual const TMetaData& GetOutputMetaData() const override final;
        virtual void AddRequester(TLink::TPtr link) override final;

    protected:
        virtual bool Convert(TDataSet::TPtr input, TDataSet::TPtr& output, IReplier::TPtr replier) const override final;
        virtual bool Accepted(TDataSet::TPtr data) const = 0;

    private:
        class TLinkFilter;
    };
}
