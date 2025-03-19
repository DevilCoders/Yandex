#include "converter.h"

namespace NCommonProxy {

    TConverter::TConverter(const TString& name, const TProcessorsConfigs& configs)
        :TProcessor(name, configs)
    {}


    void TConverter::DoStart() {
    }

    void TConverter::DoStop() {
    }

    void TConverter::DoWait() {
    }

    void TConverter::DoProcess(TDataSet::TPtr input, IReplier::TPtr replier) const {
        try {
            TVector<TDataSet::TPtr> output;
            if (ConvertMultiple(input, output, replier) && output) {
                for (auto outData: output) {
                    SendRequestToListeners(outData, replier);
                }
            }
        } catch (...) {
            replier->AddReply(GetName(), 500, CurrentExceptionMessage());
        }
    }

    bool TConverter::Convert(TDataSet::TPtr /*input*/, TDataSet::TPtr& /*output*/, IReplier::TPtr /*replier*/) const {
        return false;
    }

    bool TConverter::ConvertWithReply(TDataSet::TPtr input, TDataSet::TPtr& output, IReplier::TPtr& replier) const {
        return Convert(input, output, replier);
    }

    bool TConverter::ConvertMultiple(TDataSet::TPtr input, TVector<TDataSet::TPtr>& output, IReplier::TPtr& replier) const {
        TDataSet::TPtr outputData;
        if (ConvertWithReply(input, outputData, replier) && outputData) {
            output.push_back(outputData);
            return true;
        }
        return false;
    }

}
