#include "reqserializer.h"

TSysFileRequestSerializer::TSysFileRequestSerializer(TFile& f)
    : F(f)
{
}

TSysFileRequestSerializer::TSysFileRequestSerializer(const char* fName, bool readOnly)
    : F(fName, readOnly ? RdOnly : CreateAlways | WrOnly)
{
}

TSysFileRequestSerializer::~TSysFileRequestSerializer() {
}

size_t TSysFileRequestSerializer::DoRead(void* ptr, size_t len) {
    return F.Read(ptr, len);
}

void TSysFileRequestSerializer::DoWrite(const void* ptr, size_t len) {
    F.Write(ptr, len);
}
