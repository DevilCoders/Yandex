#include "transf_file.h"

#include <library/cpp/deprecated/fgood/fgood.h>

#include <util/memory/tempbuf.h>
#include <util/stream/file.h> // for test_transf only
#include <util/stream/output.h>

void TransferDataLimited(TFILEPtr &in, IOutputStream &out, size_t head, size_t tail) {
    TTempBuf copyBuf;
    if (in.GetLength() < (ssize_t)((head + tail + 1500) * 9 / 8)) {
        TransferData(&in, &out, copyBuf);
        return;
    }
    TString fullLine;
    if (head) {
        TransferData(&in, &out, copyBuf, head);
        // TODO: the line might be really long and we need to limit this its possible length: head / 8 + 80
        // Right now this in is not available for TFILEPtr, however for small 'tail' it is not a big problem.
        in.ReadLine(fullLine);
        out << fullLine << '\n';
    }
    off_t tailStart = in.GetLength() - tail;
    off_t tailRead = tailStart - tail / 8 - 80;
    if (tailRead > in.ftell()) {
        out << "=============== <cut> ===============\n";
        in.Seek(tailRead, SEEK_SET);
        while (in.ftell() < tailStart)
            in.ReadLine(fullLine);
        out << fullLine << '\n';
    }
    TransferData(&in, &out, copyBuf);
}

ui64 TransferData(TFILEPtr* in, IOutputStream* out, TTempBuf& b, off_t numToRead) {
    ui64   totalRead = 0;
    size_t readCount;
    char *buf = b.Data();
    size_t sz = b.Size();

    while (numToRead && (readCount = in->Read(buf, Min<off_t>(numToRead, sz))) != 0) {
        out->Write(buf, readCount);
        totalRead += readCount;
        numToRead -= readCount;
    }

    return totalRead;
}

int test_transf(const char *inName, const char *outName) {
    if (!inName || !outName) return 1;
    TFILEPtr in(inName, "r");
    TUnbufferedFileOutput out(outName);
    TransferDataLimited(in, out, 2048, 1024);
    return 0;
}
