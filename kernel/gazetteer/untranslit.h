#pragma once

#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/untranslit/untranslit.h>


namespace NGzt
{

class TRefCountedUntransliter: public TSimpleRefCount<TRefCountedUntransliter> {
public:
    TRefCountedUntransliter() {
    }

    TRefCountedUntransliter(TAutoPtr<TUntransliter> slave)
        : Slave(slave.Release())
    {
    }

    TUntransliter::WordPart GetNextAnswer() const {
        return Slave.Get() != nullptr ? Slave->GetNextAnswer() : TUntransliter::WordPart();
    }

    void Reset(const TUtf16String& text) {
        if (Slave.Get() != nullptr)
            Slave->Init(text);
    }

private:
    THolder<TUntransliter> Slave;
};


// Simple iterator over all un-transliterations of word

class TUntranslitIterator {
    static const size_t MAX_UNTRANSLITS = 5;
public:
    TUntranslitIterator()
        : Count(0)
    {
    }

    TUntranslitIterator(ELanguage lang, const TUtf16String& word = TUtf16String())
        : Untransliter(new TRefCountedUntransliter(NLemmer::GetLanguageByIdAnyway(lang)->GetUntransliter(word)))
        , Count(0)
    {
        Next();
    }

    bool Ok() const {
        return !CurrentVariant.Empty();
    }

    void operator++() {
        ++Count;
        if (Count < MAX_UNTRANSLITS)
            Next();
        else
            CurrentVariant = TUntransliter::WordPart();
    }

    TUtf16String operator*() const {
        return CurrentVariant.GetWord();
    }

    void Reset(const TUtf16String& text) {
        Y_ASSERT(Untransliter.Get() != nullptr);
        Untransliter->Reset(text);
        Count = 0;
        Next();
    }

private:
    void Next() {
        CurrentVariant = Untransliter->GetNextAnswer();
    }

private:
    TIntrusivePtr<TRefCountedUntransliter> Untransliter;
    TUntransliter::WordPart CurrentVariant;
    size_t Count;
};


}   // namespace NGzt
