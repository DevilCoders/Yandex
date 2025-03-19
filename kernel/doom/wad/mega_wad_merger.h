#pragma once

#include "mega_wad.h"
#include "mega_wad_writer.h"

namespace NDoom {

class TMegaWadMerger {
public:
    TMegaWadMerger(IOutputStream* output, IOutputStream* debug = nullptr);

    ~TMegaWadMerger();

    void Reset(IOutputStream* output);

    //! Add wad to the list of inputs. You must use std::move(wad) here.
    void Add(THolder<IWad> wad, const TString& name);

    //! Open IWad and add to the list of inputs
    void Add(const TString& name);

    //! Add IWad to the list of inputs with name IWad-n.
    void Add(THolder<IWad> wad);

    //! Process all IWads and write merged WAD.
    void Finish();

    //! Check if wad contains all the lumps from input list and compare them.
    // If out != nullptr check uses out for messages else if Debug_ != nullptr then Debug_ will be used.
    bool Check(IWad* wad, IOutputStream *out = nullptr);

private:
    struct TChunk;
    TVector<TChunk> Chunks_;
    IOutputStream* Output_ = nullptr;
    IOutputStream* Debug_ = nullptr;
};

}
