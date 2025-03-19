#pragma once

#include "gramhelper2.h"

namespace NFioExtractor {

    class INameHandler
    {
    public:
        virtual ~INameHandler() { }
        virtual void StartFio() = 0;
        virtual void ProcessFioMember(const TUtf16String& lemma,
                                      ENameType nameType,
                                      TPosting pos,
                                      bool insertInitial) = 0;

        virtual void UpdateFioPos(TPosting begin, TPosting end) = 0;

        virtual void FinishFio() = 0;
    };

}
