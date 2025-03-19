#include "options.h"

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/util/temp_table.h>
#include <mapreduce/yt/interface/operation.h>
#include <library/cpp/containers/comptrie/comptrie_builder.h>
#include <library/cpp/packers/packers.h>
#include <library/cpp/charset/wide.h>
#include <kernel/facts/dist_between_words/trie_data.h>

using namespace NYT;
using namespace NDistBetweenWords;

int main(int argc, const char** argv) {
    Initialize(argc, argv);

    TOptions opts(argc, argv);
    TWordDistTrieBuilder builder;

    IClientPtr client = CreateClient("hahn");

    auto reader = client->CreateTableReader<TNode>(opts.InputTable);
    int i = 0;

    for (; reader->IsValid(); reader->Next()) {
        auto &row = reader->GetRow();
        auto freq = opts.FreqThreshold == 0 ? 1 : row["freq"].AsDouble();

        if (freq > opts.FreqThreshold) {
            auto data = TTrieData{
                    1 - static_cast<float>(row["host5"].AsDouble()),
                    1 - static_cast<float>(row["host10"].AsDouble()),
                    1 - static_cast<float>(row["urls5"].AsDouble()),
                    1 - static_cast<float>(row["urls10"].AsDouble())
            };

            auto& word1 = row["word1"].AsString();
            auto& word2 = row["word2"].AsString();
            TUtf16String key{BuildKey(word1, word2)};
            builder.Add(key, data);
        }

        if (++i % 1'000'000 == 0) {
            Cout << i << Endl;
        }
    }

    Cout << "Total entry: " << builder.GetEntryCount() << " nodes:" << builder.GetNodeCount() << Endl;

    TUnbufferedFileOutput out(opts.OutputFile);
    builder.Save(out);

}


