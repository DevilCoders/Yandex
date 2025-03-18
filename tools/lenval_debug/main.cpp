#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/string/escape.h>

int main() {
    ui32 length;
    while (Cin.Load(&length, sizeof(length)) == sizeof(length)) {
        TString data;
        data.ReserveAndResize(length);
        Cin.LoadOrFail((void*)data.data(), length);
        Cout
            << "Length: " << length
            << "\nData:\n" << EscapeC(data) << Endl;
    }
}
