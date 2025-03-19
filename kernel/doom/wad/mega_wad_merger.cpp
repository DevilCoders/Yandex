#include "mega_wad_merger.h"
#include <util/stream/output.h>
#include <util/stream/file.h>

namespace NDoom {

struct TMegaWadMerger::TChunk {
    TChunk() = default;

    TChunk(THolder<IWad> wad, const TString& filename) {
        Wad.Swap(wad);
        DocLumps = Wad->DocLumps();
        GlobalLumps = Wad->GlobalLumps();
        Name = filename;
        Mapping.resize(DocLumps.size());
        Wad->MapDocLumps(DocLumps, Mapping);
    }

    TString Name;
    THolder<IWad> Wad = nullptr;
    TVector<TWadLumpId> DocLumps;
    TVector<TWadLumpId> GlobalLumps;
    TVector<size_t> Mapping;
};

TMegaWadMerger::TMegaWadMerger(IOutputStream* output, IOutputStream* debug)
    : Debug_(debug)
{
    Reset(output);
}

TMegaWadMerger::~TMegaWadMerger() {
}

void TMegaWadMerger::Reset(IOutputStream* output) {
    Output_ = output;
    Chunks_.resize(0);
}

void TMegaWadMerger::Add(THolder<IWad> wad, const TString& name) {
    Chunks_.emplace_back(std::move(wad), name);
}

void TMegaWadMerger::Add(const TString& name) {
    THolder<IWad> wad = IWad::Open(name);
    Add(std::move(wad), name);
}

void TMegaWadMerger::Add(THolder<IWad> wad) {
    TString name = TString("IWad-") + ToString(Chunks_.size() + 1);
    Add(std::move(wad), name);
}

class TDebugOutput {
public:
    TDebugOutput(IOutputStream* e)
        : Out_(e)
    {
    }

    template <typename T>
    TDebugOutput& operator<<(const T& a) {
        if (Out_)
            *Out_ << a;

        return *this;
    }

    template <typename T>
    TDebugOutput& operator<<(const TVector<T>& vec) {
        if (Out_) {
            *Out_ << "[ ";
            for (const T& x : vec)
                *Out_ << x << ", ";
            *Out_ << "]";
        }

        return *this;
    }

private:
    IOutputStream* Out_ = nullptr;
};

void TMegaWadMerger::Finish() {
    Y_ENSURE(Output_ != nullptr);
    Y_ENSURE(!Chunks_.empty(), "No input files");

    TDebugOutput out(Debug_);

    /* Check if all lumps are unique. */
    TMap<TWadLumpId, TString> wadNameByGlobalLump;
    TMap<TWadLumpId, TString> wadNameByDocLump;
    for (const TChunk& chunk : Chunks_) {
        for (TWadLumpId type : chunk.DocLumps) {
            Y_ENSURE_EX(!wadNameByDocLump.contains(type),
                        yexception() << "lump with type '" << ToString(type) << "' was found in " << wadNameByDocLump[type] << " and " << chunk.Name << ".");

            wadNameByDocLump[type] = chunk.Name;
        }

        for (TWadLumpId type : chunk.GlobalLumps) {
            Y_ENSURE_EX(!wadNameByGlobalLump.contains(type),
                        yexception() << "lump with type '" << ToString(type) << "' was found in " << wadNameByGlobalLump[type] << " and " << chunk.Name);

            wadNameByGlobalLump[type] = chunk.Name;
        }
    }

    /* Calc doc count. */
    ui32 docCount = 0;
    for (const TChunk& chunk : Chunks_)
        docCount = std::max(docCount, chunk.Wad->Size());

    TMegaWadWriter writer(Output_);

    /* Register index & lump types. */
    for (const TChunk& chunk : Chunks_) {
        /* Note that we're registering these in the same order as they appeared in source wads. */
        for(TWadLumpId type: chunk.Wad->DocLumps())
            writer.RegisterDocLumpType(type);
    }

    /* Save global lumps. */
    size_t writtenLumpCount = 0;
    for (const TChunk& chunk : Chunks_) {
        for (TWadLumpId type : chunk.GlobalLumps) {
            TBlob blob = chunk.Wad->LoadGlobalLump(type);
            writer.WriteGlobalLump(type, TArrayRef<const char>(blob.AsCharPtr(), blob.Size()));

            out << "\rWriting Global Lumps. [" << ++writtenLumpCount << "/" << wadNameByGlobalLump.size() << "]";
        }
    }
    out << ": Done" << Endl;

    /* Save doc lumps. */
    for (ui32 docId = 0; docId < docCount; docId++) {
        for (const TChunk& chunk : Chunks_) {
            if (docId >= chunk.Wad->Size())
                continue;

            TVector<TArrayRef<const char>> regs;
            regs.resize(chunk.Mapping.size());
            TBlob temp = chunk.Wad->LoadDocLumps(docId, chunk.Mapping, regs);

            for (size_t i = 0; i < regs.size(); i++)
                writer.WriteDocLump(docId, chunk.DocLumps[i], regs[i]);
        }

        if ((!(docId % 100) || docId > docCount - 100))
            out << "\rWriting Doc Lumps:  [" << docId + 1 << "/" << docCount << "]";
    }
    out << ": Done" << Endl;

    out << "Flushing..." << Endl;
    writer.Finish();
    Output_->Finish();
}

inline std::pair<bool, bool> CmpReg(const TArrayRef<const char>& r1, const TArrayRef<const char>& r2) {
    std::pair<bool, bool> res(false, false);
    res.first = (r1.size() == r2.size());
    if (res.first)
        res.second = (memcmp(r1.data(), r2.data(), r1.size()) == 0);
    return res;
}

inline std::pair<bool, bool> CmpBlob(const TBlob& r1, const TBlob& r2) {
    std::pair<bool, bool> res(false, false);
    res.first = (r1.Size() == r2.Size());
    if (res.first)
        res.second = (memcmp(r1.Data(), r2.Data(), r1.Size()) == 0);
    return res;
}

bool TMegaWadMerger::Check(IWad* megawad, IOutputStream* debug) {
    bool ok = true;

    if (!debug)
        debug = Debug_;

    TDebugOutput out(debug);

    for (const TChunk& chunk : Chunks_) {
        for (TWadLumpId lump : chunk.GlobalLumps) {
            if (!megawad->HasGlobalLump(lump)) {
                ok = false;
                out << "    " << chunk.Name << " doesn't have lump " << ToString(lump) << Endl;
                continue;
            }

            auto cmp = CmpBlob(chunk.Wad->LoadGlobalLump(lump), megawad->LoadGlobalLump(lump));

            if (!cmp.first) {
                ok = false;
                out << "    " << chunk.Name << ": global lump sizes are different for " << ToString(lump) << Endl;
                continue;
            }

            if (!cmp.second) {
                ok = false;
                out << "    " << chunk.Name << ": global lumps are different for " << ToString(lump) << Endl;
                continue;
            }

            out << "[" << chunk.Name << ":" << ToString(lump) << "] ....... Ok" << Endl;
        }

        TVector<TWadLumpId> wadLumps = chunk.Wad->DocLumps();

        TVector<size_t> megaMapping(wadLumps.size());
        megawad->MapDocLumps(wadLumps, megaMapping);

        TVector<size_t> wadMapping(wadLumps.size());
        megawad->MapDocLumps(wadLumps, wadMapping);

        TVector<TArrayRef<const char>> wadRegions(wadLumps.size());
        TVector<TArrayRef<const char>> megaRegions(wadLumps.size());
        out << "\nChecking " << chunk.Name << "\n"
                << " mapping is ";
        out << wadMapping << " and megawad mapping is ";
        out << megaMapping << " for lumps ";
        out << wadLumps << Endl;

        for (ui32 docId = 0; docId < chunk.Wad->Size(); docId++) {
            TBlob chunkWadBlob = chunk.Wad->LoadDocLumps(docId, wadMapping, wadRegions);
            TBlob megaWadBlob = megawad->LoadDocLumps(docId, megaMapping, megaRegions);

            for (size_t i = 0; i < wadLumps.size(); i++) {
                if (!CmpReg(wadRegions[i], megaRegions[i]).second) {
                    ok = false;
                    out << "[" << ToString(wadLumps[i]) << "] for document " << docId << " is different" << Endl;
                }
            }

            if (!(docId % 100) || docId > chunk.Wad->Size() - 100)
                out << "\rComparing Doc Lumps:  [" << docId + 1 << "/" << megawad->Size() << "]";
        }
        out << " : DONE!                            " << Endl;
    }

    return ok;
}

} // NDoom
