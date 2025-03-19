 // Generator of kernel/web_factors_info/factors_gen.{cpp,h}.
#include <kernel/generated_factors_info/metadata/factors_codegen_base.h>

void GenCode(NFactor::TCodeGenInput& input, TCodegenParams& params) {
    if (params.CppParts == 0) {
        // if it was not set from command line, set reasonable default
        params.CppParts = 30;
    }

    TWebCodegen codegen;
    codegen.GenCode(input, params);
}

int main(int argc, const char **argv) {
    return MainImpl<NFactor::TCodeGenInput>(argc, argv);
}
