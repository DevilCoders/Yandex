#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/charset/wide.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/generic/map.h>

void PrintHelp()
{
    Cout << "pass 3 file names: 1 - prefixes, 2 - roots, 3 - output file name" << Endl;
}

int main(int argc, char* argv[])
{
    if(argc != 4)
    {
        PrintHelp();
        return 0;
    }

    TMap<TString, bool> dictionary;
    TString str;
    TFileInput fPrefixes(argv[1]);
    while(fPrefixes.ReadLine(str))
    {
        dictionary[str] = false;
    }

    TFileInput fRoots(argv[2]);
    while(fRoots.ReadLine(str))
    {
        dictionary[str] = true;
    }

    TCompactTrieBuilder<wchar16, bool> builder;
    for (const auto& s : dictionary) {
        builder.Add(UTF8ToWide(s.first), s.second);
    }

    TFixedBufferFileOutput output(argv[3]);
    CompactTrieMinimizeAndMakeFastLayout(output, builder, /*verbose = */ false);
    output.Finish();

    return 0;
}

