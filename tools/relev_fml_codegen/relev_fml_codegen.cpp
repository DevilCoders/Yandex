#include <kernel/relevfml/code_gen/relev_fml_codegen.h>
#include <kernel/web_factors_info/factor_names.h>

int main(int argc, char* argv[]) {
    const IFactorsInfo* factorsInfo = GetWebFactorsInfo();
    return codegen_main(argc, argv, factorsInfo);
}
