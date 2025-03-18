#include <kernel/tarc/iface/tarcio.h>

#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <library/cpp/containers/spars_ar/spars_ar.h>
#include <util/stream/file.h>
#include <util/system/defaults.h>

#include <stdio.h>
#include <stdlib.h>

const size_t BUFFER_SIZE = 32 * 1024 * 1024;

// This utility renumbers indexarc with search numeration docIds (using index{arr,iarr} mapping), and fixes indexdir accordingly
// With '-r' key, it restores index{arc,dir} to their original state
void RenumberArc2Srch(TString indexName, TString newArcName, TString newDirName) {
    TString oldArcName = indexName + "arc",
           oldDirName = indexName + "dir",
           iarrName = indexName + "iarr",
           arrName = indexName + "arr";

    // Renumber XXXarc
    {
        TArchiveIterator oldArc(BUFFER_SIZE);
        oldArc.Open(oldArcName.data());

        TFileMappedArray<ui32> iarr; // archive -> search
        iarr.Init(iarrName.data());

        TFixedBufferFileOutput newArc(newArcName, BUFFER_SIZE);

        WriteTextArchiveHeader(newArc);

        while (TArchiveHeader* record = oldArc.NextAuto()) {
            record->DocId = iarr[record->DocId]; // Patch docId
            newArc.Write(record, record->DocLen);
        }
        newArc.Finish();
    }

    // Renumber XXXdir
    {
        TFileMappedArray<ui64> oldDir;
        oldDir.Init(oldDirName.data());

        TFileMappedArray<ui32> arr; // search -> archive
        arr.Init(arrName.data());

        TFixedBufferFileOutput newDir(newDirName, BUFFER_SIZE);

        for (ui32 searchDocId = 0; searchDocId < arr.size(); searchDocId++) {
            ui64 offset = oldDir[arr[searchDocId]];
            newDir.Write(&offset, sizeof(offset));
        }

        newDir.Finish();
    }
}

void RenumberSrch2Arc(TString indexName, TString newArcName, TString newDirName) {
    TString  oldArcName = indexName + "arc",
            oldDirName = indexName + "dir",
            iarrName = indexName + "iarr",
            arrName = indexName + "arr";

    // Renumber XXXarc
    {
        TArchiveIterator oldArc(BUFFER_SIZE);
        oldArc.Open(oldArcName.data());

        TFileMappedArray<ui32> arr; // search -> archive
        arr.Init(arrName.data());

        TFixedBufferFileOutput newArc(newArcName, BUFFER_SIZE);

        WriteTextArchiveHeader(newArc);

        while (TArchiveHeader* record = oldArc.NextAuto()) {
            record->DocId = arr[record->DocId]; // Patch docId
            newArc.Write(record, record->DocLen);
        }

        newArc.Finish();
    }

    // Renumber XXXdir
    {
        TFileMappedArray<ui64> oldDir;
        oldDir.Init(oldDirName.data());

        TFileMappedArray<ui32> iarr; // archive -> search
        iarr.Init(iarrName.data());

        TFixedBufferFileOutput newDir(newDirName, BUFFER_SIZE);

        for (ui32 arcDocId = 0; arcDocId < iarr.size(); arcDocId++) {
            ui32 searchDocId = iarr[arcDocId];
            // There may be a hole (0xFF..) in iarr mapping, which must be reproduced as a hole in dir mapping
            ui64 offset = searchDocId == Max<ui32>() ? Max<ui64>() : oldDir[searchDocId];
            newDir.Write(&offset, sizeof(offset));
        }

        newDir.Finish();
    }
}

void usage() {
    printf("Usage:\n");
    printf("\tarcdocidstrip [-r] indexName newArcFileName newDirFileName\n");
    printf("\n");
    printf("\t-r : restore archive docIds\n");
    exit(1);
}

int main(int argc, char** argv) {
    if (argc < 2)
        usage();

    if (TString("-r") == argv[1]) {
        if (argc != 5)
            usage();
        RenumberSrch2Arc(TString(argv[2]), TString(argv[3]), TString(argv[4]));
    } else {
        if (argc != 4)
            usage();
        RenumberArc2Srch(TString(argv[1]), TString(argv[2]), TString(argv[3]));
    }

    return 0;
}

