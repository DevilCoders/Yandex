#include "sorter.h"

#include <util/string/printf.h>
#include <util/stream/file.h>
#include <util/stream/zlib.h>

#include <cstdlib>

namespace NSorterPrivate {
    int CountPortions(NSorter::IPortionFileNameCallback* fname) {
        int i = 0;
        while (1) {
            FILE* f = fopen(fname->GetPortionName(i).data(), "rb");
            if (!f)
                break;
            fclose(f);
            i++;
        }
        return i;
    }

    struct TCompressedInputHelper {
        inline TCompressedInputHelper(const TString& filename, size_t bufferSize)
            : File(filename)
            , Decompressor(&File, ZLib::ZLib, bufferSize)
        {
        }

        TUnbufferedFileInput File;
        TZLibDecompress Decompressor;
    };

    struct TCompressedInput: public TCompressedInputHelper, public TBufferedInput {
        inline TCompressedInput(const TString& filename, size_t bufferSize)
            : TCompressedInputHelper(filename, bufferSize)
            , TBufferedInput(&Decompressor, bufferSize)
        {
        }
    };

    IInputStream* HeapIterInput(const TString& fname, size_t bufSize, bool compressed) {
        if (compressed) {
            return new TCompressedInput(fname, bufSize);
        }

        return new TFileInput(fname, bufSize);
    }

    class TPortionFileNameCallback: public NSorter::IPortionFileNameCallback {
    public:
        TString GetPortionName(int n) override {
            char name[100];
            //sprintf(name, "portion-%d-%d", getpid(), n);
            sprintf(name, "portion-%d", n);
            return name;
        }
    };

    static TPortionFileNameCallback PortionFilenameSimple;
}

namespace NSorter {
    TSorterBase::TSorterBase(const size_t el_sz, bool compressed)
        : Flags(0)
        , Compressed(compressed)
        , CurPortion(0)
        , ElementSize(el_sz)
        , PortionFilename(&NSorterPrivate::PortionFilenameSimple)
    {
        Store = nullptr;
        StorePos = StoreCaps = 0;
    }

    void TSorterBase::Restart() {
        //m.b. put this into a separate function
        if (!(Flags & KEEP_PORTIONS_FILES)) {
            for (int i = 0; i < CurPortion; i++) {
                remove(PortionFilename->GetPortionName(i).data());
            }
        }
        CurPortion = 0;
        StorePos = 0;
    }

    void TSorterBase::Allocate(size_t capacity, void* ext_alloc) {
        if (ext_alloc) {
            if (Flags & IS_INTERNAL_ALLOCATION)
                free(Store);
            Store = ext_alloc;
            Flags &= ~IS_INTERNAL_ALLOCATION;
        } else {
            if (Flags & IS_INTERNAL_ALLOCATION)
                Store = realloc(Store, ElementSize * capacity);
            else
                Store = malloc(ElementSize * capacity);
            Flags |= IS_INTERNAL_ALLOCATION;
        }
        if (!Store)
            ythrow yexception() << "TSorter buffer is empty";
        StorePos = 0;
        StoreCaps = capacity;
    }

    void TSorterBase::Deallocate() {
        if (Flags & IS_INTERNAL_ALLOCATION)
            free(Store);
        Flags &= ~IS_INTERNAL_ALLOCATION;
        Store = nullptr;
    }

    TSorterBase::~TSorterBase() {
        Deallocate();
        Restart();
        if (Flags & DELETE_PORTION_FNAME_OBJ)
            delete PortionFilename;
    }

    void TSorterBase::WritePortion() {
        size_t sz = StorePos * ElementSize;
        TString fname = PortionFilename->GetPortionName(CurPortion);
        TFixedBufferFileOutput fOut(fname);
        if (Compressed) {
            TZLibCompress compressor(&fOut, ZLib::ZLib, 3);
            compressor.Write(Store, sz);
        } else {
            fOut.Write(Store, sz);
        }
    }

    void TSorterBase::SetFileNameCallback(IPortionFileNameCallback* f, bool dont_delete) {
        if (Flags & DELETE_PORTION_FNAME_OBJ)
            delete PortionFilename;
        PortionFilename = f;
        if (dont_delete)
            Flags &= ~DELETE_PORTION_FNAME_OBJ;
        else
            Flags |= DELETE_PORTION_FNAME_OBJ;
    }

    TString TPortionFileNameCallback2::GetPortionName(int n) {
        TString name;
        sprintf(name, "%s-portion-%d", Cookie.data(), n);
        return name;
    }
}
