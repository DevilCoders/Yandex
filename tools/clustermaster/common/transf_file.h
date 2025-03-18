#pragma once

#include <util/generic/ylimits.h>
#include <util/system/defaults.h>

class IOutputStream;
class TFILEPtr;
class TTempBuf;

void TransferDataLimited(TFILEPtr &in, IOutputStream &out, size_t head, size_t tail);
ui64 TransferData(TFILEPtr* in, IOutputStream* out, TTempBuf& b, off_t numBytes = Max<off_t>());
