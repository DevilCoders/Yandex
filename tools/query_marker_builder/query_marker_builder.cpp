#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/string/cast.h>


#include <library/cpp/query_marker/query_marker_trie.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        Cout << "query_market_builder <input text UTF8 encoded file> <output binary file>" << Endl;
        return 1;
    }
    TIFStream input(argv[1]);
    TQueryMarkerTrie<TUtf16String> queryMarker;
    TString line;
    while (input.ReadLine(line)) {
        size_t pos = line.find('\t');
        queryMarker.LoadMarker(line.substr(0, pos), FromString<int>(line.substr(pos + 1)));
    }
    queryMarker.FinishInit();
    queryMarker.Save(argv[2]);
}
