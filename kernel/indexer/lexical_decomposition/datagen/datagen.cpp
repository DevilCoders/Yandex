#include <kernel/indexer/lexical_decomposition/vocabulary_builder.h>

#include <library/cpp/charset/codepage.h>

#include <library/cpp/getopt/opt2.h>

#include <util/generic/yexception.h>
#include <util/stream/output.h>

using namespace NLexicalDecomposition;

int main_exc(int argc, const char* argv[]) {
    Opt2 opt(argc, (char**)argv, "i:o:d:l:e:");
    const char* inputFileName = opt.Arg('i', "input data file", nullptr);
    const char* outputFileName = opt.Arg('o', "output data file");
    const char* dictFileName = opt.Arg('d', "vocabulary text file, one line per word");
    const char* language = opt.Arg('l', "language of vocabulary");
    const char* encoding = opt.Arg('e', "encoding of vocabulary", "cp1251");
    opt.AutoUsageErr("");

    TMultiVocabularyBuilder builder;
    builder.LoadVocabulary(LanguageByName(language), dictFileName, CharsetByName(encoding));
    TFixedBufferFileOutput output(outputFileName);
    if (!inputFileName)
        builder.Save(output);
    else
        builder.Update(TBlob::FromFile(inputFileName), output);
    return 0;
}

int main(int argc, const char* argv[]) {
    try {
        return main_exc(argc, argv);
    } catch (...) {
        fprintf(stderr, "exception: %s\n", CurrentExceptionMessage().data());
        return 1;
    }
}
