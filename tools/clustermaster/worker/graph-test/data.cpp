#include "data.h"

namespace {
    static const unsigned char data[] = {
        #include "test_data.inc"
    };
}

TStringBuf NWorkerGraphTestData::GetData() {
    return TStringBuf((const char*)data, sizeof(data));
}
