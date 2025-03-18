#include "http_static.h"

static const unsigned char data[] = {
    #include "http_static.inc"
};

TSolverStaticReader::TSolverStaticReader()
    : TStaticReader(data, sizeof(data))
{
}
