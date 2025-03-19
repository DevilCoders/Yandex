#pragma once

#include "token_types.h"
#include "detector.h"
#include "tokenizer.h"
#include "printer.h"


namespace NAddressFilter {

class TFilterOpts {
public:
    TTokenizerOpts TokenizerOpts;
    bool PrintResult;
    bool GoodStreets;
    int  GoLeft;
    int  GoRight;

public:
    TFilterOpts()
        : PrintResult(false),
          GoodStreets(false),
          GoLeft(0),
          GoRight(0)
    {

    }
};


class TFilter : public IAddressFilterTokenizerCallback {

public:
    TFilter(const TFilterOpts& Opts, TSimpleSharedPtr<IAddressFilterPrinter>& printer)
        : FilterOpts(Opts),
          Printer(printer),
          Detector(Opts.GoodStreets),
          Tokenizer(Opts.TokenizerOpts, *this)
    {
        if (Printer.Get() == nullptr)
            FilterOpts.PrintResult = false;
    }

    TFilter(const TFilterOpts& Opts)
        : FilterOpts(Opts),
          Detector(Opts.GoodStreets),
          Tokenizer(Opts.TokenizerOpts, *this)
    {
        FilterOpts.PrintResult = false;
    }


    typedef TVector< std::pair<size_t, size_t> > TFilterResult;

    TAutoPtr<TFilterResult> ProcessLine(const TUtf16String& text) {
        Result = TAutoPtr<TFilterResult>(new TFilterResult());

        if (FilterOpts.PrintResult)
            Printer->StartPrintResults();

        Tokenizer.ProcessLine(text);

        if (FilterOpts.PrintResult)
            Printer->EndPrintResults();

        return Result;
    }

    void OnText(const TUtf16String& sentenceText, const TVector<NToken::TTokenInfo>& tokens,  const TVector<TTokenType>& tokenTypes) override;


private:
    TFilterOpts FilterOpts;
    TSimpleSharedPtr<IAddressFilterPrinter>  Printer;

    TDetector Detector;
    TTokenizer Tokenizer;

    TAutoPtr<TFilterResult> Result;
};

} //NAddressFilter
