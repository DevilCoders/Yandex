#include <util/system/defaults.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/generic/hash.h>
#include <util/ysaveload.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <utility>

int main(int argc, char* argv[])
{
    Y_UNUSED(argc);
    Y_UNUSED(argv);
    THashMap<TString, std::pair<TUtf16String, TUtf16String> > url2data;
    TString url;
    TUtf16String title, desc;
    while (Cin.ReadLine(url) && Cin.ReadLine(title) && Cin.ReadLine(desc)) {
        url2data[url] = std::make_pair(title, desc);
    }
    TString b64;
    TStringStream out;
    Save(&out, url2data);
    Base64Encode(out.Str(), b64);
    Cout << "TString data = \"" << b64 << "\";" << Endl;
}
