#include <util/stream/output.h>

#include <library/cpp/lua/eval.h>
#include <library/cpp/config/config.h>

using namespace NConfig;


int main() {
    TConfig c = TConfig::FromLua(Cin, TGlobals());
    c.DumpJson(Cout);
}

