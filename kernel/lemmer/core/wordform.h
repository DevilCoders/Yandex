#pragma once

#include "lemmer.h"
#include <util/generic/noncopyable.h>

// Declared in this file:
class TWordformKit;
class TDefaultWordformIterator;

class TWordformKit {
public:
    TWordformKit(const TYandexLemma& lemma, size_t flexIndex);

    bool IsEmpty() const {
        return Lemma == nullptr;
    }

    const char* GetStemGram() const {
        return (Lemma != nullptr) ? Lemma->GetStemGram() : nullptr;
    }

    const char* GetFlexGram() const {
        return FlexGrammar;
    }

    inline bool HasSameStemParts(const TWordformKit& a) const {
        return StemBase == a.StemBase && StemFlexion == a.StemFlexion;
    }

    inline bool HasSameParts(const TWordformKit& a) const {
        return HasSameStemParts(a) && InvertedFlexion == a.InvertedFlexion && DirectFlexion == a.DirectFlexion &&
               Prefix == a.Prefix && Postfix == a.Postfix;
    }

    bool HasSameText(const TWordformKit& a) const;

    size_t StemSize() const {
        return StemBase.size() + StemFlexion.size();
    }

    size_t FlexionSize() const {
        return InvertedFlexion.size() + DirectFlexion.size();
    }

    size_t TotalSize() const {
        return Prefix.size() + StemSize() + FlexionSize() + Postfix.size();
    }

    ui32 GetWeight() const {
        return Weight;
    }

    size_t CommonPrefixLength(const TWtringBuf& str) const;
    size_t CommonPrefixLength(const TWordformKit& kit) const;

    TWtringBuf ConstructText(wchar16* buffer, size_t bufferSize) const;
    void ConstructText(TUtf16String& text) const;
    void ConstructForm(TYandexWordform& form) const;

protected:
    inline TWordformKit()
        : Lemma(nullptr)
        , FlexGrammar(nullptr)
        , Weight(0)
    {
    }

    void SetDefaultText(const TYandexLemma& lemma, const TWtringBuf& text);

    friend class TDefaultWordformIterator;

    friend class TWordformCharIterator;

protected:
    const TYandexLemma* Lemma; // should outlive @this

    TWtringBuf Prefix;
    TWtringBuf StemBase;        // stem without stem-flexion
    TWtringBuf StemFlexion;     // modified from stem to stem
    TWtringBuf InvertedFlexion; // modified within single stem, reversed.
    TWtringBuf DirectFlexion;   // same as InvertedFlexion, but without inversion.
    // Usually only one of InvertedFlexion or DirectFlexion is defined.
    // But if both are non-empty they are ordered: first InvertedFlexion, then DirectFlexion
    TWtringBuf Postfix; // "+" for "Europe+"

    const char* FlexGrammar;
    ui32 Weight;
};

// base interface for wordform iterators
class IWordformIterator {
public:
    virtual inline ~IWordformIterator() {
    }

    virtual void operator++() = 0;
    virtual bool IsValid() const = 0;

    inline const TWordformKit& operator*() const {
        return GetValue();
    }

    inline const TWordformKit* operator->() const {
        return &GetValue();
    }

    virtual size_t FormsCount() const = 0;

private:
    virtual const TWordformKit& GetValue() const = 0;
};

class TDefaultWordformIterator: public IWordformIterator, TNonCopyable {
public:
    explicit TDefaultWordformIterator(const TYandexLemma& lemma);

    // implements IWordformIterator

    inline bool IsValid() const override {
        return Current != nullptr;
    }

    void operator++() override {
        Current = (Current == &Lemma && !Form.HasSameParts(Lemma)) ? &Form : nullptr;
    }

    size_t FormsCount() const override {
        return 1;
    }

private:
    inline const TWordformKit& GetValue() const override {
        Y_ASSERT(Current != nullptr);
        return *Current;
    }

    TWordformKit Lemma, Form;
    TWordformKit* Current;
};
