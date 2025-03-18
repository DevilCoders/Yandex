#include "config.h"
#include "generator.h"
#include "main.h"

int NReverseGeocoder::NGenerator::main(int argc, const char* argv[]) {
    const TConfig initParams(argc, argv);
    NReverseGeocoder::NGenerator::Generate(initParams);
    return EXIT_SUCCESS;
}
