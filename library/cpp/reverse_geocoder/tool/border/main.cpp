#include "extractor.h"
#include "main.h"

int NReverseGeocoder::NBorderExtractor::main(int argc, const char* argv[]) {
    const TInitParams initParams(argc, argv);
    Extract(initParams);
    return EXIT_SUCCESS;
}
