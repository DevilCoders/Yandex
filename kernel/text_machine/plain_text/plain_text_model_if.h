#pragma once

#include <kernel/text_machine/interface/hit.h>

namespace NTextMachine {
namespace NPlainText {
    // Interface that computes similarity score given a query and set of document
    // annotations (like title or snippet).
    class IPlainTextModel {
    public:
        virtual ~IPlainTextModel() {}

        virtual const char* Name() = 0;
        virtual float Compute(const TString& query,
                const TVector<TAnnotation>& docAnnotations) = 0;
    };

    using TPlainTextModelsVector = TVector<IPlainTextModel*>;
} // NPlainText
} // NTextMachine
