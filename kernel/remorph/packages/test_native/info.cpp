#include <iostream>
#include <yandex/remorph_wrap.h>

using namespace NRemorphAPI;

int main() {
    TInfoWrap info = GetRemorphInfoWrap();
    std::cout << "Remorph: " << info.GetMajorVersion() << '.' << info.GetMinorVersion() << '.' << info.GetPatchVersion() << std::endl;
    return 0;
}


