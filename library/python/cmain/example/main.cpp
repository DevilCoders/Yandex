#include "main.h"

#include <util/generic/xrange.h>
#include <util/system/types.h>
#include <util/stream/output.h>

int RunExampleMain(int argc, char** argv) {
    Cout << "Hello World!" << Endl;
    for (size_t i : xrange(argc)) {
        Cout << argv[i] << Endl;
    }
    return 0;
}
