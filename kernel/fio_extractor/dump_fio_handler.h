#pragma once

#include "name_handler.h"

namespace NFioExtractor {

    class TFioDumper : public INameHandler
    {
    public:
        TFioDumper(INameHandler* handler, const TUtf16String& prefix)
            : Handler(handler)
            , Prefix(prefix)
        {
        }

        void StartFio() override;

        void ProcessFioMember(const TUtf16String& lemma,
                              ENameType nameType,
                              TPosting pos,
                              bool insertInitial) override;

        void UpdateFioPos(TPosting begin, TPosting end) override;

        void FinishFio() override;

        void Dump(IOutputStream& stream) const;

    private:

        struct TPart {
            ENameType type;
            TUtf16String value;
        };

        friend bool operator< (const TPart& lhs, const TPart& rhs);

        INameHandler* Handler;
        TUtf16String Prefix;
        TVector<TPart> Parts;
        THashMap<TString, size_t> Counter;
    };

}
