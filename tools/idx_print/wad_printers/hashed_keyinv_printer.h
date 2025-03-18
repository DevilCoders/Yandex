#pragma once

#include <kernel/doom/offroad_ngrams_panther_wad/offroad_ngrams_panther_wad_io.h>
#include <search/panther/runtime/term_hashing/term_hashing.h>
#include <tools/idx_print/utils/options.h>

#include <util/string/hex.h>

template<class Io>
void PrintInvHashWad(const TIdxPrintOptions& options) {
    using THash = typename Io::TReader::THash;
    using THit = typename Io::TReader::THit;
    using TReader = typename Io::TReader;
    if (!options.DocIds.empty()) {
        Cerr << "not supported yet\n";
        return;
    }
    TString path = options.IndexPath;
    if (path.EndsWith('.')) {
        path.pop_back();
    }
    TReader reader(path + ".block.wad", path + ".hit.wad");
    THash hash;
    THit hit;
    TString out(2 * sizeof(hash), '\0');
    while (reader.ReadHash(&hash)) {
        NPanther::BigEndianHexEncode(&hash, sizeof(hash), out);
        Cout << out << '\n';
        if (options.PrintHits) {
            while (reader.ReadHit(&hit)) {
                Cout << '\t' << hit << '\n';
            }
        }
    }
}
