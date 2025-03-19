#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/ysafeptr.h>
////////////////////////////////////////////////////////////////////////////////////////////////////
struct IRecognizerFactory;
struct IPhraseRecognizer;
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NYawk {

    // rule result
    struct TResultPart
    {
        TString TokenName;
        TUtf16String TokenValue;
    };
    typedef TVector<TResultPart> TResult;

    struct IResultConsumer {
        virtual void NewMatch(size_t numParts) = 0;
        virtual void Part(size_t partIdx, const TString &token, const TUtf16String &value) = 0;
        virtual void EndMatch() = 0;
        virtual ~IResultConsumer() {
        }
    };

    // rule
    class TRule: public IObjectBase
    {
        OBJECT_METHODS(TRule);
        //
        TVector<TString> TokenNames;
        TVector<TVector<TUtf16String> > Tokens;
        TObj<IRecognizerFactory> RecognizerFactory;
        mutable TVector<TObj<IPhraseRecognizer> > Recognizers; // not serialized

        void AddToken(const TString &name, const TUtf16String &description);
        void AddOptionalToken(const TUtf16String &description);
        void Init() const;
        //
    public:
        TRule(IRecognizerFactory *factory = nullptr);

        bool IsMatch(const TUtf16String &phrase, bool ordered) const;
        bool Recognize(const TUtf16String &phrase, bool ordered, IResultConsumer *res) const;
        bool Recognize(const TUtf16String &phrase, bool ordered, TVector<TResult> *res) const;

        bool HasToken(const TString &name) const;
        void GetUsedFileNames(TVector<TString> *res) const;

        void CreateRule(const TVector<TString> &tokens, bool isUtf);
    };

}
////////////////////////////////////////////////////////////////////////////////////////////////////
