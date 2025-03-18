#include <tools/printtrie/lib/main.h>

#include <util/generic/yexception.h>
#include <util/stream/output.h>

int main(const int argc, const char* argv[]) {
    try {
        return NTrieOps::MainPrint(argc, argv);
    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
    }
    return EXIT_FAILURE;
}
