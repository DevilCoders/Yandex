#include "main.h"

#include <library/cpp/getopt/small/modchooser.h>

#include <tools/printtrie/lib/main.h>
#include <tools/triecompiler/lib/main.h>
#include <tools/trietest/lib/main.h>

int NTrieOps::Main(const int argc, const char* argv[]) {
    TModChooser app;
    app.SetDescription(
        "comptrie [1] ops\n"
        "[1] https://a.yandex-team.ru/arc/trunk/arcadia/library/comptrie/README.md\n");
    app.AddMode(
        "test",
        MainTest,
        "Filters input, leaving only lines contained in a trie and adding leaf values");
    app.AddMode(
        "print",
        MainPrint,
        "Print trie in CSV format");
    app.AddMode(
        "compile",
        MainCompile,
        "Build trie from CSV formatted file");
    return app.Run(argc, argv);
}
