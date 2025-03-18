#pragma once

#include <util/stream/input.h>
#include <util/stream/output.h>
#include <util/system/types.h>
#include <util/generic/vector.h>


TString Compress(const TString&);
TString Decompress(const TString&);


ui64 CompressFile(const TString& src, const TString& dst);
ui64 DecompressFile(const TString& src, const TString& dst);


struct TSvnCommit {
    ui32 revision;
    TString author;
    TString date;
    TString msg;
    TString tree;
    TString parent;
};


struct TTreeEntry {
    TString name;
    ui8 mode;
    ui64 size;
    TString hash;
    TVector<TString> blobs;
};


struct TTree {
    TVector<TTreeEntry> entries;
};


struct THgHead {
    TString branch;
    TString hash;
};


struct THgHeads {
    TVector<THgHead> heads;
};


struct THgChangeset {
    TString hash;
    TString author;
    TString date;
    TString msg;
    TVector<TString> files;
    TString branch;
    ui8 close_branch;
    TString extra;
    TString tree;
    TVector<TString> parents;
};


TString DumpSvnCommit(const TSvnCommit&);
TSvnCommit LoadSvnCommit(const TString&);

TString DumpTree(const TTree&);
TTree LoadTree(const TString&);

TString DumpHgHeads(const THgHeads&);
THgHeads LoadHgHeads(const TString&);

TString DumpHgChangeset(const THgChangeset&);
THgChangeset LoadHgChangeset(const TString&);
