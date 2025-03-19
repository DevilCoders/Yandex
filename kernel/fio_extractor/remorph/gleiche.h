#pragma once

#include <kernel/remorph/input/input_symbol.h>

#include <kernel/fio_extractor/fiowordsequence.h>
#include <kernel/fio_extractor/fioclusterbuilder.h>

namespace NFioExtractor {

    bool GleicheFio(const TFullFIO& fullFio, const TFIOOccurenceInText& fioOcc,
                    TVector<TFullFIO>& fullFios2, TVector<TFIOOccurenceInText>& fioOccs2,
                    const TVector<NSymbol::TInputSymbol*>& inputSymbols);

    template<class TFioFactCallback>
    class TFioFactGleiche {
    public:
        explicit TFioFactGleiche(TFioFactCallback& callback)
            : Callback(callback)
        {
        }

        template<class InputSymbolContainer>
        void operator () (const InputSymbolContainer& symbols,
                          const TFullFIO& fullFio,
                          const TFIOOccurenceInText& fioOcc,
                          bool isCoherent)
        {
            if (isCoherent) {
                Callback(symbols, fullFio, fioOcc, isCoherent);
                return;
            }
            TVector<TFullFIO> fullFios2;
            TVector<TFIOOccurenceInText> fioOccs2;
            if (GleicheFio(fullFio, fioOcc, fullFios2, fioOccs2, Adapt(symbols))) {
                Y_ASSERT(fullFios2.size() == fioOccs2.size());
                for (size_t j = 0; j < fullFios2.size(); j++)
                    Callback(symbols, fullFios2[j], fioOccs2[j], true);
            } else {
                // we fail to find consistent variants, pass fio as is
                Callback(symbols, fullFio, fioOcc, false);
            }
        }

    private:
        TFioFactCallback& Callback;

        template<class InputSymbolPtrContainer>
        static TVector<NSymbol::TInputSymbol*> Adapt(const InputSymbolPtrContainer& inputSymbols) {
            TVector<NSymbol::TInputSymbol*> symbols;
            symbols.reserve(inputSymbols.size());
            typedef typename InputSymbolPtrContainer::const_iterator iterator;
            for (iterator ptr = inputSymbols.begin(); ptr != inputSymbols.end(); ++ptr)
                symbols.push_back(ptr->Get());
            return symbols;
        }
    };
}
