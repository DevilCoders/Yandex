#pragma once

#include "processor.h"

namespace NCommonProxy {

    class TConverter : public TProcessor {
    public:
        TConverter(const TString& name, const TProcessorsConfigs& configs);
        virtual void DoStart() override;
        virtual void DoStop() override;
        virtual void DoWait() override;

    protected:
        void DoProcess(TDataSet::TPtr input, IReplier::TPtr replier) const override final;
        virtual bool Convert(TDataSet::TPtr input, TDataSet::TPtr& output, IReplier::TPtr replier) const;
        virtual bool ConvertWithReply(TDataSet::TPtr input, TDataSet::TPtr& output, IReplier::TPtr& replier) const;
        virtual bool ConvertMultiple(TDataSet::TPtr input, TVector<TDataSet::TPtr>& output, IReplier::TPtr& replier) const;
    };
}
