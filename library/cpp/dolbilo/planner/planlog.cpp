#include "planlog.h"

TPlanLoader::TPlanLoader() {
}

TPlanLoader::~TPlanLoader() {
}

class TDevastateItemAdder {
    public:
        inline TDevastateItemAdder(IReqsLoader::TParams* parent) noexcept
            : Parent_(parent)
        {
        }

        inline ~TDevastateItemAdder() {
        }

        inline EVerdict operator() (const TDevastateItem& item) {
            Parent_->Add(item);
            return V_CONTINUE;
        }

    private:
        IReqsLoader::TParams* Parent_;
};

void TPlanLoader::Process(TParams* params) {
    TDevastateItemAdder adder(params);

    ForEachPlanItem(params->Input(), adder);
}
