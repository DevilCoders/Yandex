#pragma once

#include "token_types.h"
#include "detector.h"
#include "tokenizer.h"

#include <util/stream/output.h>
#include <library/cpp/json/writer/json.h>

namespace NAddressFilter {

struct IAddressFilterPrinter {
    virtual void PrintText(const TUtf16String& sentenceText) = 0;

    virtual void StartPrintTextResult(size_t start, size_t end, TWtringBuf result) = 0;

    virtual void EndPrintTextResult() = 0;

    virtual void StartPrintResults() = 0;

    virtual void EndPrintResults() = 0;

    virtual void PrintToken(TWtringBuf token) = 0;

    virtual void PrintCoords(size_t start, size_t end) = 0;

    virtual ~IAddressFilterPrinter(){
    }
};

class TSimplePrinter: public IAddressFilterPrinter {

public:
    TSimplePrinter(IOutputStream& stream)
        : Stream(stream)
    {
    }

    void PrintText(const TUtf16String& sentenceText) override{
        Y_UNUSED(sentenceText);
    }


    void StartPrintTextResult(size_t start, size_t end, TWtringBuf result) override{
        Stream << start << " " << end << " " << result << "\t";
    }

    void EndPrintTextResult() override {
        Stream << "\t";
    }

    void StartPrintResults() override{
    }

    void EndPrintResults() override {
        Stream << Endl;
    }

    void PrintToken(TWtringBuf token) override {
        Stream << token << " ";
    }

    void PrintCoords(size_t start, size_t end) override {
        Stream << start << " " << end << "\t";
    }

    virtual void EndPrintResult() {
    }

    ~TSimplePrinter() override {
    }

private:
    IOutputStream&  Stream;
};

class TJsonPrinter: public IAddressFilterPrinter {

public:
    TJsonPrinter(IOutputStream& stream)
        : Stream(stream)
         , CommaIsNeeded(false)
    {
    }

    void PrintText(const TUtf16String& sentenceText) override{
        Y_UNUSED(sentenceText);
    }


    void StartPrintResults() override{
        Stream << "[ ";
        CommaIsNeeded = false;
    }
    void EndPrintResults() override {
        Stream << " ] ";
        Stream << Endl;
        CommaIsNeeded = false;
    }

    void StartPrintTextResult(size_t start, size_t end, TWtringBuf result) override{
        if (CommaIsNeeded)
            Stream << ", ";
        Stream << "{";
        Stream << "\"start\": " << start;
        Stream << ", ";
        Stream << "\"end\": " << end;
        Stream << ", ";
        Stream << "\"answer\": \"" << result << "\"";
        Stream << ", ";
        Stream << "\"tokens\": [";
        CommaIsNeeded = false;
    }

    void EndPrintTextResult() override {
        Stream << "] ";
        Stream << "}";
        CommaIsNeeded = true;
    }

    void PrintToken(TWtringBuf token) override {
        if (CommaIsNeeded)
            Stream << ", ";
        Stream << "\"" << token << "\"";
        CommaIsNeeded = true;
    }

    void PrintCoords(size_t start, size_t end) override {
        if (CommaIsNeeded)
            Stream << ", ";
        Stream << "{";
        Stream << "\"start\": " << start;
        Stream << ", ";
        Stream << "\"end\": " << end;
        Stream << "}";
        CommaIsNeeded = true;
    }

    ~TJsonPrinter() override {
    }

private:
    IOutputStream&  Stream;
    bool            CommaIsNeeded;
};

class TCorpusPrinter: public IAddressFilterPrinter {

public:
    TCorpusPrinter(IOutputStream& stream)
        : Stream(stream)
         , EOLIsNeeded(false)
    {
    }

    void PrintText(const TUtf16String& sentenceText) override{
        Stream << sentenceText;
        EOLIsNeeded = true;
    }

    void StartPrintResults() override{
    }

    void EndPrintResults() override {
        if (EOLIsNeeded)
            Stream << Endl;
        EOLIsNeeded = false;
    }

    void StartPrintTextResult(size_t start, size_t end, TWtringBuf result) override{
        Y_UNUSED(start);
        Y_UNUSED(end);
        Y_UNUSED(result);
        Stream << "\t";
    }

    void EndPrintTextResult() override {
    }

    void PrintToken(TWtringBuf token) override {
        Stream << token << " ";
    }

    void PrintCoords(size_t start, size_t end) override {
        Y_UNUSED(start);
        Y_UNUSED(end);
    }

    ~TCorpusPrinter() override {
    }

private:
    IOutputStream&  Stream;
    bool            EOLIsNeeded;
};

}
