#pragma once

#include "lemmer.h"
#include "language.h"

#include <library/cpp/token/nlptypes.h>
#include <library/cpp/token/token_structure.h>

#include <util/generic/vector.h>
#include <utility>

namespace NLemmer {
    /*!Second parameter is the weight of the lemma (disambiguation is taken into account),
     * zero if disambiguation wasn't applied.
     */
    typedef std::pair<TYandexLemma, double> TLemmerResult;
    typedef TVector<TLemmerResult> TLemmerResults;

    class ILemmerResultsHandler {
    public:
        //! @returns true if results was used in handler
        virtual bool OnLemmerResults(const TWideToken& token, NLP_TYPE type, const TLemmerResults* results) = 0;

        virtual void Flush() = 0;

        virtual ~ILemmerResultsHandler(){};
    };

}
