#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "generate.h"

#include <library/cpp/getopt/small/last_getopt.h>
#include <library/cpp/resource/resource.h>

#include <util/stream/file.h>

int main(int argc, const char** argv) {
    using namespace NDomSchemeCompiler;

    TString inFileName;
    TString outFileName;

    {
        NLastGetopt::TOpts opts;

        opts.AddLongOption("in")
            .Required()
            .StoreResult(&inFileName);
        opts.AddLongOption("out")
            .Required()
            .StoreResult(&outFileName);

        NLastGetopt::TOptsParseResult optsRes(&opts, argc, argv);
    }

    try {
        TProgram program = Parse(inFileName);

        TFixedBufferFileOutput out(outFileName);
        out << "#pragma once\n\n" << NResource::Find("/runtime") << "\n";
        Generate(program, out);
    } catch (const TParseError& pe) {
        Cerr << pe.Position.FileName << ":" << pe.Position.Line << ":" << pe.Position.Offset << ": "<< pe.Message << "\n";
        return 1;
    } catch (...) {
        Cerr << argv[0] << ": " << CurrentExceptionMessage() << "\n";
        return 1;
    }

    return 0;
}
