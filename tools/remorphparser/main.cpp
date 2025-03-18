#include "handler.h"
#include "options.h"

#include <util/generic/yexception.h>

int main(int argc, char* argv[]) {
    try {
        NRemorphParser::TRunOpts opts(argc, argv);
        NRemorphParser::ProcessDocs(opts);
    } catch (const yexception& error) {
        Cerr << "ERROR: " << error.what() << Endl;
        return 2;
    }
    return 0;
}
