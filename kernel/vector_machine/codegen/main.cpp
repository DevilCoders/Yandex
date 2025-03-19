#include "common.h"

#include <kernel/vector_machine/codegen/config.pb.h>

#include <library/cpp/protobuf/util/pb_io.h>

#include <util/stream/file.h>
#include <util/system/compiler.h>


int main(int argc, char** argv)
{
    Y_UNUSED(argc);

    TFileInput configFile(argv[1]);
    /* will be empty, but ya make requires it */
    TFileOutput cppFile(argv[2]);
    TFileOutput headerFile(argv[3]);

    NVectorMachine::TDssmBoostingConfig config;
    ParseFromTextFormat(configFile, config);

    NVectorMachine::TDssmBoostingGenerator(config).Output(headerFile);
    return 0;
}
