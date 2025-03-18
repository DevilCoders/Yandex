#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>

#include <kernel/doom/wad/wad.h>

#include <tools/idx_print/utils/options.h>

using TLumpSet = TSet<NDoom::TWadLumpId>;
using TLumps = TVector<NDoom::TWadLumpId>;

class TBaseWadPrinter {
public:
    TBaseWadPrinter(const TIdxPrintOptions& options, NDoom::IWad* wad)
        : Options_(options)
        , Wad_(wad)
    {
    }
    void Print(ui32 docId, IOutputStream* out = &Cout) {
        DoPrint(docId, out);

    }
    void Print(IOutputStream* out = &Cout) {
        PrintKeys(out);
    }
    virtual const char* Name() = 0;
    virtual ~TBaseWadPrinter() {}

private:
    virtual void PrintKeys(IOutputStream* out) {
        *out << "DUMMY_KEY\n";
    }

    virtual void DoPrint(ui32 docId, IOutputStream* out) = 0;

protected:
    bool CheckKey(const TStringBuf& key) const {
        return (Options_.ExactQuery && key == Options_.Query)
            || (!Options_.ExactQuery && key.StartsWith(Options_.Query));
    }

protected:
    const TIdxPrintOptions& Options_;
    NDoom::IWad* Wad_;
};

