#pragma once

#include <library/cpp/containers/comptrie/comptrie.h>
#include <kernel/lemmer/dictlib/grammar_enum.h>
#include <util/stream/output.h>
#include <util/system/types.h>
#include <util/generic/ptr.h>

namespace NNewLemmer {
    using TFlexTrie = TCompactTrie<wchar16, ui32>;
    using TPatternsTrie = TCompactTrie<wchar16, ui32>;
    using TDiaMask = ui16;

    const wchar16* GetEmptyFlex();

    class TGrammarIterator {
    public:
        TGrammarIterator(const char* data)
            : Data(data)
        {
        }

        bool IsValid() const {
            return Data && *Data != '\0';
        }

        const EGrammar* AsGrammarPtr() const {
            return reinterpret_cast<const EGrammar*>(Data);
        }

        EGrammar GetGrammar() const {
            Y_ASSERT(IsValid());
            return (EGrammar)(*Data);
        }

        EGrammar operator*() const {
            return GetGrammar();
        }

        void Next() {
            if (IsValid()) {
                ++Data;
            } else {
                Y_ASSERT(false);
            }
        }

        TGrammarIterator& operator++() {
            Next();
            return *this;
        }

    private:
        const char* Data = nullptr;
    };

    class TBinaryGrammarString {
    public:
        TBinaryGrammarString(const char* data = nullptr)
            : Data(data)
        {
        }

        bool IsValid() const {
            return Data;
        }

        const char* AsCharPtr() const {
            return Data;
        }

        TGrammarIterator GetGrammarIterator() const {
            return {Data};
        }

        TGrammarIterator begin() const {
            return GetGrammarIterator();
        }

        TGrammarIterator end() const {
            TGrammarIterator end = GetGrammarIterator();
            while (end.IsValid()) {
                ++end;
            }
            return end;
        }

        bool HasGram(EGrammar grammar) const {
            for (TGrammarIterator it = begin(); it.IsValid(); it.Next()) {
                if ((unsigned char)*(it.AsGrammarPtr()) == (unsigned char)grammar) {
                    return true;
                }
            }
            return false;
        }

    private:
        const char* Data = nullptr;
    };

    class IGrammarStringIterator {
    public:
        virtual ~IGrammarStringIterator() = default;
        virtual bool IsValid() const = 0;
        virtual void Next() = 0;
        virtual TBinaryGrammarString Get() const = 0;

        operator bool() const {
            return IsValid();
        }

        IGrammarStringIterator& operator++() {
            Next();
            return *this;
        }

        TBinaryGrammarString operator*() const {
            return Get();
        }
    };

    using IGrammarStringIteratorPtr = THolder<IGrammarStringIterator>;

    class IBlock {
    public:
        virtual ~IBlock() = default;
        virtual ui32 GetFrequency() const = 0;
        virtual const wchar16* GetFormFlex() const = 0;
        virtual IGrammarStringIteratorPtr GetGrammarStringIterator() const = 0;
        virtual bool HasNext() const = 0;
    };

    using IBlockPtr = THolder<IBlock>;

    class IScheme {
    public:
        virtual ~IScheme() = default;
        virtual size_t GetId() const = 0;
        virtual ui32 GetFrequency() const = 0;
        virtual TFlexTrie GetFlexTrie() const = 0;
        virtual const wchar16* GetLemmaFlex() const = 0;
        virtual TBinaryGrammarString GetGrammarString() const = 0;
        virtual IBlockPtr GetBlock(size_t blockId) const = 0;
    };

    using ISchemePtr = THolder<IScheme>;

    class IPattern {
    public:
        virtual ~IPattern() = default;
        virtual ISchemePtr GetScheme() const = 0;
        virtual ui8 GetStemLen() const = 0;
        virtual ui8 GetOldFlexLen() const = 0;
        virtual bool HasPrefix() const = 0;
        virtual bool IsFinal() const = 0;
        virtual ui8 GetRest() const = 0;
        virtual bool IsUseAlways() const = 0;
        virtual ui32 GetFrequency() const = 0;
        virtual TDiaMask GetDiaMask() const = 0;
        virtual bool HasFlexTrie() const = 0;
        virtual TFlexTrie GetFlexTrie() const = 0;
    };

    using IPatternPtr = THolder<IPattern>;

    class IPatternIterator {
    public:
        virtual ~IPatternIterator() = default;
        virtual bool IsValid() const = 0;
        virtual void Next() = 0;
        virtual IPatternPtr Get() const = 0;

        operator bool() const {
            return IsValid();
        }

        IPatternIterator& operator++() {
            Next();
            return *this;
        }

        IPatternPtr operator*() const {
            return Get();
        }
    };

    using IPatternIteratorPtr = THolder<IPatternIterator>;

    struct TPatternMatchResult {
        size_t MatchedLength = 0;
        IPatternIteratorPtr PatternIterator;
    };

    class IDictData {
    public:
        virtual ~IDictData() = default;
        virtual TPatternMatchResult GetMatchedPatterns(TWtringBuf revWord) const = 0;
        virtual ISchemePtr GetScheme(size_t schemeId) const = 0;
        virtual TString GetFingerprint() const = 0;
    };
}
