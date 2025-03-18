#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/charset/unidata.h>
#include <util/string/vector.h>
#include <util/string/printf.h>
#include <util/charset/wide.h>
#include <util/string/split.h>

int main(int argc, char* argv[]) {
    Y_UNUSED(argc);
    Y_UNUSED(argv);

    TUtf16String space(u" ");
    TUtf16String line;

    TVector<TUtf16String> splitted;
    int i(0);
    while(Cin.ReadLine(line)) {
        StringSplitter(line).SplitByString(space.data()).SkipEmpty().Collect(&splitted);
        if (splitted.empty()) {
            continue;
        }

        Cout << Sprintf("const ui64 SEQ%i[] = {", i);
        for (TVector<TUtf16String>::const_iterator cit = splitted.begin(); cit != splitted.end(); cit++) {
            const TUtf16String& s = *cit;
            Cout << Sprintf("ULL(%lu), ", ComputeHash(s));
        }
        Cout << Sprintf("}; // %s", WideToUTF8(line).data()) << Endl;
        i++;
    }
    return 0;
}
