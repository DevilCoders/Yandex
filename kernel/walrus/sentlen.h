#pragma once

#include <kernel/sent_lens/sent_lens_writer.h>

typedef TVector<TSentenceLengths> TIndexLens;
typedef TVector<ui16> TNumBreaks;




class TTitleBreaks
{
public:
    TVector<std::pair<ui16, ui16>> FirstLastBrk;
    bool ValidKey;
public:
    TTitleBreaks()
        : ValidKey(false)
    {
    }

    void SetKey(const char* key) {
        ValidKey = key[0] != '#' && key[0] != '(' && key[0] != ')';
    }

    void AddHit(TWordPosition wp) {
        if (!ValidKey)
            return;
        if (!wp.InTitle())
            return;
        ui32 doc = wp.Doc();
        if (doc >= FirstLastBrk.size()) {
            size_t newSz = 3 * doc / 2 + 1;
            FirstLastBrk.resize(newSz, std::make_pair(std::numeric_limits<ui16>::max(), (ui16)0));
        }
        FirstLastBrk[doc].first = Min(FirstLastBrk[doc].first, wp.Break());
        FirstLastBrk[doc].second = Max(FirstLastBrk[doc].second, wp.Break());
    }

    void Save(ui32 maxDocId, const char* name) {
        TFixedBufferFileOutput fout(name);
        ui32 version = 1;
        SavePodType(&fout, version);
        SavePodType(&fout, maxDocId + 1);
        FirstLastBrk.resize(maxDocId + 1);
        SavePodArray(&fout, FirstLastBrk.data(), FirstLastBrk.size());
    }
};


class TSentLens
{
public:
    TIndexLens Result;
    TNumBreaks NumBreaks;
    ui32 MaxDocId;
    bool ValidKey;

public:
    TSentLens()
        : MaxDocId(0)
        , ValidKey(false)
    {
    }

    void SetKey(const char* key) {
        ValidKey = key[0] != '#' && key[0] != '(' && key[0] != ')';
    }

    void AddHit(TWordPosition wp) {
        // SAAS-4771: we need [possibly empty] entries for all docs, even if they have no text hits
        if (wp.Doc() >= Result.size()) {
            Result.resize(3*wp.Doc()/2 + 1);
            NumBreaks.resize(Result.size());
        }
        MaxDocId = Max(MaxDocId, wp.Doc());

        if (!ValidKey)
            return;

        TSentenceLengths& lens = Result[wp.Doc()];
        if (wp.Break() >= lens.ysize())
            lens.resize(3*wp.Break()/2 + 1);
        NumBreaks[wp.Doc()] = Max(NumBreaks[wp.Doc()], (ui16)(wp.Break() + 1));
        ui8& len = lens[wp.Break()];
        len = Max(len, (ui8)wp.Word());
    }

    void Save(const char* name) {
        if (MaxDocId > 0) {
            for (size_t i = 0; i <= MaxDocId; ++i)
                Result[i].resize(NumBreaks[i]);
            Result.resize(MaxDocId + 1);
        }
        TSentenceLengthsWriter writer(name, TSentenceLengthsReader::SL_VERSION);
        ui32 tsize = 0;
        for (ui32 docId = 0; docId < Result.size(); ++docId) {
            writer.Add(docId, Result[docId]);
            tsize += Result[docId].size();
        }
        fprintf(stderr, "Sent size: %i\n", tsize);
    }
};

class TFakeLens
{
public:
    void SetKey(const char*) {
    }

    void AddHit(TWordPosition, ui32) {
    }
};

class TSentMultiWriter {
    TVector<TSentLens> SL;
    TVector<TTitleBreaks> TB;
public:
    TSentMultiWriter(ui32 destCount)
        : SL(destCount)
        , TB(destCount)
    {
    }

    void SetKey(const char* key) {
        for (ui32 i = 0 ; i < SL.size(); ++i) {
            SL[i].SetKey(key);
            TB[i].SetKey(key);
        }
    }

    void AddHit(TWordPosition wp, ui32 dest) {
        Y_ASSERT(dest < SL.size());

        SL[dest].AddHit(wp);
        TB[dest].AddHit(wp);
    }

    void Save(const char* name, ui32 dest) {
        Y_ASSERT(dest < SL.size());

        SL[dest].Save(name);
        TB[dest].Save(SL[dest].MaxDocId, (TString(name) + ".title").data());
    }
};
