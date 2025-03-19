#include "options.h"

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/operation.h>
#include <ysite/yandex/pure/pure.h>
#include <library/cpp/charset/wide.h>
#include <library/cpp/containers/comptrie/comptrie.h>

#include <util/string/join.h>
#include <util/string/strip.h>
#include <util/generic/vector.h>
#include <util/generic/xrange.h>

#include <utility>

using namespace NYT;
using namespace NDistBetweenWords;


int main(int argc, const char** argv) {
    Initialize(argc, argv);

    TOptions opts(argc, argv);
    TCompactTrieBuilder<wchar16, bool> builder;

    IClientPtr client = CreateClient("hahn");

    auto reader = client->CreateTableReader<TNode>(opts.InputTable);
    int i = 0;

    for (; reader->IsValid(); reader->Next()) {
        auto& row = reader->GetRow();

        auto key = CharToWide(row["key"].AsString(), CODES_UTF8);
        TVector<TUtf16String> parts = StringSplitter(key).Split(wchar16(';'));
        if (parts.size() == 2) {
            for (auto& part : parts) {
                auto spaces = std::count(part.cbegin(), part.cend(), wchar16(' '));
                if (spaces <= 1) {
                    builder.Add(part, true);
                }
            }
        }
        if (++i % 1'000'000 == 0) {
            Cout << i << Endl;
        }
    }

    Cout << "Total entry: " << builder.GetEntryCount() << " nodes:" << builder.GetNodeCount() << Endl;

    TUnbufferedFileOutput out(opts.OutputFile);
    builder.Save(out);
}




