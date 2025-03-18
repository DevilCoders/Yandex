#include <tools/snipmake/hebe/lib/config.h>
#include <tools/snipmake/hebe/lib/run.h>

#include <mapreduce/lib/init.h>

#include <library/cpp/svnversion/svnversion.h>

#include <util/stream/output.h>

int main(int argc, const char* argv[]) {
    NMR::Initialize(argc, argv);
    NHebe::TConfig cfg(argc, argv);

    if (cfg.Version) {
        Cout << GetProgramSvnVersion() << Endl;
        return 0;
    }

    return NHebe::Run(cfg);
}
