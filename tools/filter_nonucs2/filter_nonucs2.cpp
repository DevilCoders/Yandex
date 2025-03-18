#include <library/cpp/deprecated/parse_utils/parse_utils.h>

int main(int /*argc*/, const char* /*argv*/[]) {
    TString l;
    while (Cin.ReadLine(l)) {
        if (!IsNonBMPUTF8(l))
            Cout << l << '\n';
    }
    return 0;
}
