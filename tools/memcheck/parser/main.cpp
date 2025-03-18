#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/folder/filelist.h>
#include <util/generic/yexception.h>

#include <tools/memcheck/common/stat.h>

static inline TString GetDir(TString s) {
    while (!s.empty() && s[s.size() - 1] != '/') {
        s.pop_back();
    }

    return s;
}

int main(int argc, char** argv) {
    try {
        if (argc < 2) {
            ythrow yexception() << "wrong params";
        }

        const TString p(argv[1]);
        const TString d(GetDir(p));

        TAllocatorStats stats;
        TFileList lst;

        lst.Fill(d.data());
        const char* name;

        while ((name = lst.Next())) {
            const TString path = d + name;

            if (!path.StartsWith(p)) {
                continue;
            }

            Cerr << "--> " << path << Endl;

            TFileInput in(path);

            try {
                while (stats.Read(&in));
            } catch (const std::exception& e) {
                Cerr << e.what() << Endl;
            } catch (...) {
                Cerr << "unknown error" << Endl;
            }
        }

        stats.Print();
    } catch (const std::exception& e) {
        Cerr << e.what() << Endl;
    }
}
