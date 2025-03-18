#pragma once

#include "calculators.h"
#include "snip2serpiter.h"

#include <tools/snipmake/common/common.h>

namespace NSnippets
{

    class TSnippetsMetricsCalculationApplication
    {
    private:
        THolder<ISerpsIterator> IteratorHolder;
        ISerpsIterator* Iterator;
        TSnipMetricsCalculator* Calculator;

    public:
        TSnippetsMetricsCalculationApplication(ISnippetsIterator* iterator, TSnipMetricsCalculator* calculator)
            : IteratorHolder(new TSnip2SerpIter(iterator))
            , Iterator(IteratorHolder.Get())
            , Calculator(calculator)
        {
        }

        TSnippetsMetricsCalculationApplication(ISerpsIterator* iterator, TSnipMetricsCalculator* calculator)
            : Iterator(iterator)
            , Calculator(calculator)
        {
        }

        int Run() {
            // Iterate through input
            while (Iterator->Next()) {
                (*Calculator)(Iterator->Get());
            }
            Calculator->Report();

            return 0;
        }
    };
}
