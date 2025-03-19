#pragma once
#include <util/generic/ptr.h>

#include <library/cpp/langmask/langmask.h>

class TBlob;

namespace NSnippets {

    class TAnswerModels {
    private:
        class TImpl;

        THolder<TImpl> Impl;

    public:
        TAnswerModels();
        explicit TAnswerModels(const TBlob& blob);
        explicit TAnswerModels(const TString& filename);
        void InitFromBlob(const TBlob& blob);
        void InitFromFilename(const TString& filename);
        bool Empty() const;
        ~TAnswerModels();
        float ApplyWord(const TUtf16String& word, ELanguage language) const;
    };

}
