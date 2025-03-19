#include "wad_writer.h"

namespace NDoom {

void IWadWriter::RegisterDocLumpType(TStringBuf /* id */) {
    Y_ENSURE(false, "Unimplemented");
}

IOutputStream* IWadWriter::StartGlobalLump(TStringBuf /* id */) {
    Y_ENSURE(false, "Unimplemented");
}

IOutputStream* IWadWriter::StartDocLump(ui32 /* doc */, TStringBuf /* id */) {
    Y_ENSURE(false, "Unimplemented");
}

} // namespace NDoom
