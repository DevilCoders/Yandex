#include <kernel/factor_slices/metadata/slices_metadata.pb.h>

#include "slices_codegen.h"


void GenCode(NFactorSlice::TCodeGenInput& input, TCodegenParams& params) {
    TFactorSlicesCodegen<NFactorSlice::TCodeGenInput> codegen;
    codegen.GenCode(input, params);
}

int main(int argc, const char **argv) {
    return MainImpl<NFactorSlice::TCodeGenInput>(argc, argv);
}
