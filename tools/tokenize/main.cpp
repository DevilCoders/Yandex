#include <kernel/qtree/request/req_node.h>
#include <kernel/qtree/request/reqscan.h>
#include <kernel/qtree/request/request.h>
#include <library/cpp/charset/wide.h>
#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/modchooser.h>
#include <library/cpp/tokenizer/tokenizer.h>
#include <util/datetime/cputimer.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/memory/tempbuf.h>
#include <util/stream/file.h>
#include <util/string/escape.h>

struct TParams {
    TString InFile;
    TString OutFile;
    IInputStream* In = &Cin;
    IOutputStream* Out = &Cout;
    THolder<IInputStream> InHolder;
    THolder<IOutputStream> OutHolder;
    size_t Version = NTokenizerVersionsInfo::Default;
    bool StopError = true;
    bool BackwardCompatible = false;
    bool QuietMode = false;
    bool EscapeMisc = true;
    bool MeasureTime = false;
    bool PrintOriginalQuery = false;
    bool Universal = false;
    TString LeftDelimiter = "[";
    TString RightDelimiter = "]";
    bool PrintRequestTree = false;
    bool AsciiSpecialTokens = false;
};

void PrintToken(const TUtf16String& token, IOutputStream& out, const TParams& params) {
    out << params.LeftDelimiter << token << params.RightDelimiter;
}

class TNullTokensHandler : public ITokenHandler {
public:
    TNullTokensHandler(IOutputStream& out)
        : Out(out)
    {}

    void OnToken(const TWideToken&, size_t, NLP_TYPE) override {
        NTokens++;
    }

    ~TNullTokensHandler() override {
        Out << NTokens << " tokens" << Endl;
    }
private:
    size_t NTokens = 0;
    IOutputStream& Out;
};

class TPrintUtfTokensHandler : public ITokenHandler {
public:
    TPrintUtfTokensHandler(IOutputStream& out, const TParams& params)
        : Out(out)
        , Params(params)
    {}

    void OnToken(const TWideToken& token, size_t origleng, NLP_TYPE type) override {
        Y_ASSERT(origleng >= token.Leng);
        if (!Params.Universal) {
            Out << type << " " << token.Leng;
            if (origleng != token.Leng)
                Out << " (" << origleng << ")";

            if (type == NLP_WORD || type == NLP_MARK || type == NLP_INTEGER || type == NLP_FLOAT) {
                if (token.SubTokens.size() > 1) {
                    for (const TCharSpan * it = token.SubTokens.begin(); it != token.SubTokens.end(); ++it)
                        PrintToken(TUtf16String(token.Token, it->Pos, it->Len), Out, Params);
                } else
                    PrintToken(TUtf16String(token.Token, token.Leng), Out, Params);
            } else {
                if (Params.EscapeMisc) {
                    PrintToken(EscapeC(TUtf16String(token.Token, token.Leng)), Out, Params);
                } else {
                    PrintToken(TUtf16String(token.Token, token.Leng), Out, Params);
                }
            }
            Out << Endl;
        } else {
            if ((type == NLP_WORD || type == NLP_INTEGER) && token.SubTokens.size() <= 1) {
                PrintToken(TUtf16String(token.Token, token.Leng), Out, Params);
                Out << Endl;
            }
        }

        if (token.SubTokens.size()) {
            Y_ASSERT(token.SubTokens.back().EndPos() <= token.Leng);
        }
    }

private:
    IOutputStream& Out;
    const TParams& Params;
};



void DocumentTokenize(IInputStream& in, IOutputStream& out, const TParams& params) {
    TUtf16String input = UTF8ToWide(in.ReadAll());
    THolder<ITokenHandler> handler = nullptr;
    if (params.QuietMode)
        handler.Reset(new TNullTokensHandler(out));
    else
        handler.Reset(new TPrintUtfTokensHandler(out, params));


    THolder<TFormattedPrecisionTimer> timelogger = nullptr;
    if (params.MeasureTime)
        timelogger.Reset(new TFormattedPrecisionTimer("nlp", &Cerr));

    TTokenizerOptions opts;
    opts.Version = params.Version;
    TNlpTokenizer(*handler, params.BackwardCompatible).Tokenize(input.c_str(), input.size(), opts);
    if (!params.Universal)
        Cout << Endl;
}

tRequest::TNodePtr TokenizeText(const TUtf16String& text, size_t version) {
    tRequest req;
    return req.Parse(text, nullptr, nullptr, version, nullptr, false);
}

/// Print all leaves in the tree.
void PrintParseTree(TRequestNode* root, const TUtf16String& sourceText, IOutputStream& out, const TParams& params) {
    if (root == nullptr) {
        return;
    }

    if (root->IsLeaf()) {
        TUtf16String token = LegacySubstr(sourceText, root->Span.Pos, root->Span.Len);
        PrintToken(token, out, params);
        if (params.Universal)
            out << Endl;
    }

    PrintParseTree(root->Left, sourceText, out, params);
    PrintParseTree(root->Right, sourceText, out, params);
}

IOutputStream& printNode(IOutputStream &os, const TRequestNode *n, int depth);
void QueryTokenize(IInputStream& in, IOutputStream& out, const TParams& params) {
    TUtf16String input;

    THolder<TFormattedPrecisionTimer> timelogger = nullptr;
    if (params.MeasureTime)
        timelogger.Reset(new TFormattedPrecisionTimer("nlp", &Cerr));

    while (in.ReadLine(input)) {
        try {
            tRequest::TNodePtr parsed = TokenizeText(input, params.Version);
            if (params.PrintRequestTree) {
                printNode(Cout, parsed.Get(), 0);
            }
            if (params.PrintOriginalQuery) {
                out << input << '\t';
            }
            PrintParseTree(parsed.Get(), input, out, params);
            if (!params.Universal)
                out << Endl;
        } catch (const yexception& e) {
            if (params.StopError) {
                throw;
            }
            out << "Error '" << e.what() << "' while parsing '" << input << "'" << Endl;
        }
    }
}

void AddQueryOptions(NLastGetopt::TOpts& opts, TParams& params) {
    opts.AddLongOption("not-stop", "don't stop on first error")
        .StoreResult(&params.StopError, false)
        .NoArgument();
    opts.AddLongOption("original", "print original query")
        .StoreResult(&params.PrintOriginalQuery, true)
        .NoArgument()
        .Help("print original query");
    opts.AddLongOption("print-tree", "print binary tree")
        .StoreResult(&params.PrintRequestTree, true)
        .NoArgument();
}

void AddDocumentOptions(NLastGetopt::TOpts& opts, TParams& params) {
    opts.AddLongOption('b', "backward-compatible")
        .StoreResult(&params.BackwardCompatible, true)
        .NoArgument();
    opts.AddLongOption('q', "quiet-mode")
        .StoreResult(&params.QuietMode, true)
        .NoArgument()
        .Help("quiet mode: only number of tokens is printed");
    opts.AddCharOption('n', "not-escape-misc")
        .StoreResult(&params.EscapeMisc, false)
        .NoArgument()
        .Help("don't escape non-word tokens");
}

void AddGeneralOptions(NLastGetopt::TOpts& opts, TParams& params) {
    opts.AddLongOption('o', "output")
        .StoreResult(&params.OutFile)
        .Help("write result to the given file")
        .Handler1T<TString>([&params](const auto& path) {
            params.OutHolder.Reset(new TFixedBufferFileOutput(path));
            params.Out = params.OutHolder.Get();
        });
    opts.AddLongOption('i', "input")
        .StoreResult(&params.InFile)
        .Help("read from the given file")
        .Handler1T<TString>([&params](const auto& path) {
            params.InHolder.Reset(new TFileInput(path));
            params.In = params.InHolder.Get();
        });
    opts.AddLongOption('u', "universal")
        .StoreResult(&params.Universal, true)
        .NoArgument()
        .Help("print in universal format");
    opts.AddLongOption("left-d", "left delimiter")
        .StoreResult(&params.LeftDelimiter)
        .Help("choose left delimiter for tokens, '[' - default");
    opts.AddLongOption("right-d", "rigth delimiter")
        .StoreResult(&params.RightDelimiter)
        .Help("choose right delimiter for tokens, ']' - default");

    opts.AddLongOption('m', "measure-time")
        .StoreResult(&params.MeasureTime, true)
        .NoArgument()
        .Help("measure tokenization time");

    opts.AddLongOption('v', "version")
        .StoreResult(&params.Version);
}

int QueryOpts(int argc, const char** argv) {
    TParams params;
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    AddGeneralOptions(opts, params);
    AddQueryOptions(opts, params);
    NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    QueryTokenize(*params.In, *params.Out, params);
    return 0;
}

int DocumentOpts(int argc, const char** argv) {
    TParams params;
    NLastGetopt::TOpts opts = NLastGetopt::TOpts::Default();
    AddGeneralOptions(opts, params);
    AddDocumentOptions(opts, params);
    NLastGetopt::TOptsParseResult res(&opts, argc, argv);
    DocumentTokenize(*params.In, *params.Out, params);
    return 0;
}

int main(int argc, const char** argv) {
    TModChooser modChooser;
    modChooser.AddMode(
        "query",
        QueryOpts,
        "Query tokenizer");
    modChooser.AddMode(
        "document",
        DocumentOpts,
        "Document tokenizer");
    modChooser.Run(argc, argv);
    return 0;
}
