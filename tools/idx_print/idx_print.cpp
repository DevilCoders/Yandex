#include <kernel/doom/index_format_processor/index_format_processor.h>

#include <library/cpp/getopt/last_getopt.h>
#include <kernel/doom/array4d/array4d_io.h>
#include <tools/idx_print/plain_printers/key_inv_printer.h>
#include <tools/idx_print/plain_printers/hnsw_printer.h>
#include <tools/idx_print/utils/options.h>
#include <tools/idx_print/wad_printers/doc_attrs_printer.h>
#include <tools/idx_print/wad_printers/hashed_keyinv_printer.h>
#include <tools/idx_print/wad_printers/smart_wad_printer.h>
#include <tools/idx_print/wad_printers/itditp_slim_index_printer.h>

#include <util/generic/set.h>
#include <util/system/yassert.h>

void ParseOptions(int argc, const char** argv, TIdxPrintOptions* options, NDoom::TIndexFormatProcessor<>& proc) {
    NLastGetopt::TOpts opts;
    opts.SetTitle("idx_print -- base index printer.");
    opts.SetFreeArgsNum(0);
    opts.AddCharOption('i', "index path").RequiredArgument("<path>").Required().StoreResult(&options->IndexPath);
    opts.AddLongOption("chunked", "chunked wad").RequiredArgument("<path>").Optional().NoArgument().SetFlag(&options->Chunked);
    opts.AddCharOption('f', "index format").RequiredArgument("<format>").StoreResult(&options->IndexFormat);
    opts.AddCharOption('q', "query prefix").RequiredArgument("<prefix>").StoreResult(&options->Query)
        .AddShortName('w');
    opts.AddCharOption('e', "print only the keys that exactly match the provided query prefix").NoArgument().SetFlag(&options->ExactQuery);
    opts.AddLongOption('H', "print-hits", "print hits").NoArgument().SetFlag(&options->PrintHits);
    opts.AddLongOption('y', "yandex-encoded", "treat keys as yandex-encoded with optional utf8 prefix").NoArgument().SetFlag(&options->YandexEncoded);
    opts.AddLongOption('x', "hex-encode-bad-chars", "works with --yandex-encoded, enables hex-encoding as a fallback to ensure that output is always utf-8").NoArgument().SetFlag(&options->HexEncodeBadChars);
    opts.AddCharOption('d', "docid set to restrict hit and key output to (to be used with -H)").RequiredArgument("<list>").RangeSplitHandler(&options->DocIds, ',', '-');
    opts.AddLongOption('p', "wad-printers", "wad printers you want to use").RequiredArgument("<list>").SplitHandler(&options->WadPrinters, ',');
    proc.RegisterHelpHandler(opts, Cerr, "help", "print usage and exit");

    Y_ENSURE(options->DocIds.empty() || options->PrintHits, "Restricting output with a doc id set is not supported without -H option.");

    NLastGetopt::TOptsParseResult parseResult(&opts, argc, argv);
}

template<class Reader>
void PrintDocIndex(const TIdxPrintOptions& options) {
    Reader reader(options.IndexPath);

    if (!options.PrintHits)
        ythrow yexception() << "Nothing to print, use -H to print hits.";

    ui32 docId;
    typename Reader::THit hit;
    while (reader.ReadDoc(&docId)) {
        if (!options.DocIds.empty() && !options.DocIds.contains(docId))
            continue;

        while (reader.ReadHit(&hit))
            Cout << "\t" << hit << "\n";
    }
}

#define REGISTER_PRINTER(indexFormat, printer) \
    proc.AddProcessor(indexFormat, [&] { printer(options); } )

int IdxPrint(int argc, const char** argv) {
    using namespace NDoom;

    TIdxPrintOptions options;
    TIndexFormatProcessor<> proc;

    REGISTER_PRINTER(Array4dIndexFormat, PrintDocIndex<TArray4dReader>);
    REGISTER_PRINTER(YandexIndexFormat, PrintKeyInvIndex<TIdentityFunctions<TYandexIo>>);
    REGISTER_PRINTER(HnswFormat, PrintHnswIndex);
    REGISTER_PRINTER(YandexCountsIndexFormat, PrintKeyInvIndex<TIdentityFunctions<TYandexCountsIo>>);
    REGISTER_PRINTER(YandexPantherIndexFormat, PrintKeyInvIndex<TIdentityFunctions<TYandexPantherIo>>);
    REGISTER_PRINTER(OffroadCountsIndexFormat, PrintKeyInvIndex<TIdentityFunctions<TOffroadCountsIo>>);
    REGISTER_PRINTER(OffroadPantherIndexFormat, PrintKeyInvIndex<TIdentityFunctions<TOffroadPantherIo>>);
    REGISTER_PRINTER(OffroadDoublePantherIndexFormat, PrintKeyInvIndex<TIdentityFunctions<TOffroadDoublePantherIo>>);
    REGISTER_PRINTER(OmniIndexFormat, PrintWad);
    REGISTER_PRINTER(MegaWadFormat, PrintWad);
    REGISTER_PRINTER(IndexAttrsFormat, PrintKeyInvIndex<TIdentityFunctions<TOffroadAttributesIo>>);
    REGISTER_PRINTER(DocAttrsFormat, PrintDocAttrsIndex);
    REGISTER_PRINTER(CategToNameFormat, PrintCategToName);
    REGISTER_PRINTER(HashedKeyInvFormat, PrintInvHashWad<TOffroadNgramsPantherIo>);
    REGISTER_PRINTER(WebItdItpSlimIndexFormat, PrintSlimIndex);

    ParseOptions(argc, argv, &options, proc);

    if (options.IndexFormat == UnknownIndexFormat) {
        options.IndexFormat = DetectIndexFormat(options.IndexPath);
    }
    Y_ENSURE(options.IndexFormat != UnknownIndexFormat, "Index format not recognized!");

    proc.Process(options.IndexFormat);
    return 0;
}


int main(int argc, const char** argv) {
    try {
        return IdxPrint(argc, argv);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}
