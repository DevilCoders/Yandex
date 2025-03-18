
#include <kernel/url_tools/url_tools.h>

#include <util/generic/string.h>
#include <util/stream/input.h>
#include <util/stream/output.h>


int main(int /*argc*/, const char* /*argv*/[])
{
    TString l;

    while (Cin.ReadLine(l)) {
        TIsUrlResult res = IsUrl(l, 0);
        Cout << res.IsUrl << Endl;
    }

    return 0;
}

