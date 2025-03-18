/// author@ cheusov@ Aleksey Cheusov
/// created: Fri, 10 Oct 2014 17:32:39 +0300
/// see: OXYGEN-901,OXYGEN-898

#include <search/pruning/pruning_data.h>
#include <util/stream/output.h>

int main(int argc, char **argv)
{
    --argc;
    ++argv;

    if (argc != 1){
        Cerr << "Usage: prngrp_index_print <indexfile>\n";
        return 1;
    }

    TPruningData data;
    if (!data.InitFromIndex(argv[0], TPruning::AUTO)){
        return 2;
    }

    Cout << "Pruning mode:" << (ui32)data.GetPruningMode() << '\n';

    ui32 count = data.GetPruningGroupsSize();
    Cout << "Pruning groups size:" << count << '\n';
    const ui16* groups = data.GetPruningGroups();
    for (ui32 i=0; i < count; ++i){
        Cout << groups[i] << '\n';
    }

    count = data.GetPruningValuesSize();
    Cout << "Pruning values size:" << count << '\n';
    const ui16* values = data.GetPruningValues();
    for (ui32 i=0; i < count; ++i){
        Cout << values[i] << '\n';
    }

    return 0;
}
