#include <kernel/lemmer/superlemmer/superlemmer.h>
#include <library/cpp/containers/comptrie/comptrie_builder.h>
#include <util/stream/input.h>
#include <util/stream/output.h>

template <typename T>
size_t CommonPrefixLen(const T& a, const T& b) {
    size_t i = 0;
    while (i < a.size() && i < b.size() && a[i] == b[i])
        ++i;
    return i;
}

using namespace NSuperLemmer;

int main() {
    TCompactTrieBuilder<char, TSuperFlex> builder;

    TString line;
    while (Cin.ReadLine(line)) {
        size_t pos = line.find('\t');
        TString s1 = line.substr(0, pos);
        TString s2 = line.substr(pos + 1);
        size_t cp = CommonPrefixLen(s1, s2);
        TString end = s2.substr(cp);
        TSuperFlex value{s1.size() - cp, end};
        builder.Add(s1.c_str(), s1.size(), value);
    }

    CompactTrieMinimize(Cout, builder);
}
