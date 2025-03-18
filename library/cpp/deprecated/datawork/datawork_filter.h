#pragma once

#include <library/cpp/deprecated/mbitmap/mbitmap.h>
#include <util/generic/hash_set.h>
#include <util/stream/file.h>

#include <library/cpp/microbdb/safeopen.h>
#include "basetypestdio.h"

struct TCmpCore {
    //strict definition do not change
    //lt  === <
    //gt  === >
    //eq  === ==
    //cmp  < === -1, == === 0, > === -1

    ui32 MVal32;
    i32 MVali32;
    ui64 MVal64;
    i64 MVali64;
    double DVal;
    float FVal;
    const char* MValSArg; //todo late string parsing using dbscheme getting type

    void Init(const char* str) {
        MValSArg = str;
        MVal32 = (ui32)atoi(str);
        MVali32 = (i32)atoi(str);
        MVal64 = 0;
        MVali64 = 0;
        const char* cpy = str;
        ReadField(MVal64, cpy);
        cpy = str;
        ReadField(MVali64, cpy);
        sscanf(str, "%lf", &DVal);
        sscanf(str, "%f", &FVal); //ReadField(FVal,    str); //not defined for double!!!
    }

    template <class TVal>
    bool lt(TVal) {
        ythrow yexception() << "lt not supported";
    } //todo late string parsing using dbscheme getting type
    bool lt(ui8 fld) {
        return fld < MVal32;
    }
    bool lt(i8 fld) {
        return fld < MVali32;
    }

    bool lt(ui16 fld) {
        return fld < MVal32;
    }
    bool lt(i16 fld) {
        return fld < MVali32;
    }

    bool lt(ui32 fld) {
        return fld < MVal32;
    }
    bool lt(i32 fld) {
        return fld < MVali32;
    }
    bool lt(ui64 fld) {
        return fld < MVal64;
    }
    bool lt(i64 fld) {
        return fld < MVali64;
    }

    bool lt(float fld) {
        return fld < FVal;
    }
    bool lt(double fld) {
        return fld < DVal;
    }
    bool lt(const char* fld) {
        return strcmp(fld, MValSArg) < 0;
    }

    template <class TVal>
    bool eq(TVal) {
        ythrow yexception() << "eq not supported";
    } //todo late string parsing using dbscheme getting type
    bool eq(ui8 fld) {
        return fld == MVal32;
    }
    bool eq(i8 fld) {
        return fld == MVali32;
    }
    bool eq(ui16 fld) {
        return fld == MVal32;
    }
    bool eq(i16 fld) {
        return fld == MVali32;
    }

    bool eq(ui32 fld) {
        return fld == MVal32;
    }
    bool eq(i32 fld) {
        return fld == MVali32;
    }
    bool eq(ui64 fld) {
        return fld == MVal64;
    }
    bool eq(i64 fld) {
        return fld == MVali64;
    }

    bool eq(float fld) {
        return fld == FVal;
    }
    bool eq(double fld) {
        return fld == DVal;
    }

    bool eq(const char* fld) {
        return strcmp(fld, MValSArg) == 0;
    }

    template <class TVal>
    bool gt(TVal) {
        ythrow yexception() << "gt not supported";
    } //todo late string parsing using dbscheme getting type
    bool gt(ui8 fld) {
        return fld > MVal32;
    }
    bool gt(i8 fld) {
        return fld > MVali32;
    }
    bool gt(ui16 fld) {
        return fld > MVal32;
    }
    bool gt(i16 fld) {
        return fld > MVali32;
    }

    bool gt(ui32 fld) {
        return fld > MVal32;
    }
    bool gt(i32 fld) {
        return fld > MVali32;
    }
    bool gt(ui64 fld) {
        return fld > MVal64;
    }
    bool gt(i64 fld) {
        return fld > MVali64;
    }

    bool gt(float fld) {
        return fld > FVal;
    }
    bool gt(double fld) {
        return fld > DVal;
    }

    bool gt(const char* fld) {
        return strcmp(fld, MValSArg) > 0;
    }

    template <class TVal>
    int cmp(TVal fld) {
        if (lt(fld))
            return -1;
        if (gt(fld))
            return 1;
        return 0;
    }
    int cmp(const char* fld) {
        return strcmp(fld, MValSArg);
    }
};

struct TEqFilter: public TCmpCore {
    typedef bool TRet;
    template <class T, class TVal>
    bool operator()(const T*, TVal fld) {
        return eq(fld);
    }
};
struct TNeFilter: public TCmpCore {
    typedef bool TRet;
    template <class T, class TVal>
    bool operator()(const T*, TVal fld) {
        return !eq(fld);
    }
};
struct TLtFilter: public TCmpCore {
    typedef bool TRet;
    template <class T, class TVal>
    bool operator()(const T*, TVal fld) {
        return lt(fld);
    }
};
struct TGtFilter: public TCmpCore {
    typedef bool TRet;
    template <class T, class TVal>
    bool operator()(const T*, TVal fld) {
        return gt(fld);
    }
};

struct TEqFile {
    bitmap_1 EqBits;
    THashSet<ui64> EqHash64;
    THashSet<double> EqHashF;
    THashSet<double> EqHashD;
    THashSet<TString> EqHashS;
    typedef bool TRet;
    const char* FileName;
    bool Loaded;
    void Init(const char* str) {
        FileName = str;
        Loaded = false;
    }

    void Load32(const char* fmt) //fmt = "%u"
    {
        FILE* f = fopen(FileName, "rb");
        if (!f)
            ythrow yexception() << "can't open filter file for read " << FileName << "\n";
        ui32 v = 0;
        while (fscanf(f, fmt, &v) == 1)
            EqBits.set(v);
        //fprintf(stderr, "%d filter ids readed\n",(int)FilterSet.size());
        fclose(f);
        Loaded = true;
    }
    void Load64(const char* fmt) //fmt = "%u"
    {
        FILE* f = fopen(FileName, "rb");
        if (!f)
            ythrow yexception() << "can't open filter file for read " << FileName << "\n";
        ui64 v = 0;
        while (fscanf(f, fmt, &v) == 1)
            EqHash64.insert(v);
        //fprintf(stderr, "%d filter ids readed\n",(int)FilterSet.size());
        fclose(f);
        Loaded = true;
    }
    void LoadF() {
        FILE* f = fopen(FileName, "rb");
        if (!f)
            ythrow yexception() << "can't open filter file for read " << FileName << "\n";
        float v = 0;
        while (fscanf(f, "%f", &v) == 1)
            EqHashF.insert(v);
        //fprintf(stderr, "%d filter ids readed\n",(int)FilterSet.size());
        fclose(f);
        Loaded = true;
    }

    void LoadD() {
        FILE* f = fopen(FileName, "rb");
        if (!f)
            ythrow yexception() << "can't open filter file for read " << FileName << "\n";
        double v = 0;
        while (fscanf(f, "%lf", &v) == 1)
            EqHashD.insert(v);
        //fprintf(stderr, "%d filter ids readed\n",(int)FilterSet.size());
        fclose(f);
        Loaded = true;
    }

    void LoadS() {
        THolder<IInputStream> fin;
        TString v;
        try {
            fin.Reset(new TFileInput(FileName));
        } catch (...) {
            ythrow yexception() << "can't open filter file for read " << FileName << "\n";
        }
        while (!!fin && fin->ReadLine(v))
            EqHashS.insert(v);
        Loaded = true;
    }

    bool Test32(ui32 val) {
        if (!Loaded)
            Load32("%u");
        return EqBits.test(val);
    }
    bool Test32(i32 val) {
        if (!Loaded)
            Load32("%d");
        return EqBits.test((ui32)val);
    }
    bool Test64(ui64 val) {
        if (!Loaded)
            Load64("%" PRIu64);
        return EqHash64.contains((ui64)val);
    }

    bool Test64(i64 val) {
        if (!Loaded)
            Load64("%" PRId64);
        return EqHash64.contains((ui64)val);
    }
    bool TestF(float val) {
        if (!Loaded)
            LoadF();
        return EqHashF.contains(val);
    }
    bool TestD(double val) {
        if (!Loaded)
            LoadD();
        return EqHashD.contains(val);
    }
    bool TestS(const char* val) {
        if (!Loaded)
            LoadS();
        return EqHashS.contains(TString(val));
    }

    template <class TVal>
    bool eq(TVal) {
        ythrow yexception() << "eq not supported";
    } //todo late string parsing using dbscheme getting type

    bool eq(ui8 fld) {
        return Test32((ui32)fld);
    }
    bool eq(i8 fld) {
        return Test32((i32)fld);
    }
    bool eq(ui16 fld) {
        return Test32((ui32)fld);
    }
    bool eq(i16 fld) {
        return Test32((i32)fld);
    }
    bool eq(ui32 fld) {
        return Test32((ui32)fld);
    }
    bool eq(i32 fld) {
        return Test32((i32)fld);
    }

    bool eq(ui64 fld) {
        return Test64((ui64)fld);
    }
    bool eq(i64 fld) {
        return Test64((i64)fld);
    }

    bool eq(float fld) {
        return TestF(fld);
    }
    bool eq(double fld) {
        return TestD(fld);
    }
    bool eq(const char* fld) {
        return TestS(fld);
    }

    //sorry strings are not supported yet
    template <class T, class TVal>
    bool operator()(const T*, TVal fld) {
        return eq(fld);
    }
};

struct TNeFile : TEqFile {
    //sorry strings are not supported yet
    template <class T, class TVal>
    bool operator()(const T*, TVal fld) {
        return !eq(fld);
    }
};

struct TBinSearchCore {
    static const size_t InputBuffer = 128 << 10;
    static const size_t OutputBuffer = 128 << 10;
    static const size_t LinearRead = 4 << 20;
    typedef int TRet;
    TLtFilter Left;
    TGtFilter Right;

    void Init(const char* left, const char* right) {
        Left.Init(left);
        Right.Init(right);
    }
    template <class T, class TVal>
    int operator()(const T*, TVal fld) {
        return cmp(fld);
    }

    template <class TRec, class TSel>
    void operator()(const TRec* rec, TInDatFileImpl<TRec>& idatfile, TOutDatFileImpl<TRec>& odatfile, const TSel& sel) {
        int left = 0;
        int right = idatfile.GetLastPage() - 1;
        rec = idatfile.GotoLastPage();

        int pages = (int)(LinearRead / idatfile.GetPageSize());

        while (right - left > pages) {
            int middle = (left + right) >> 1;
            rec = idatfile.GotoPage(middle);
            if (sel(rec, Left))
                left = middle + 1;
            else
                right = middle;
        }
        //borders check (left and right)
        if (left > 0)
            left--;

        rec = idatfile.GotoPage(left);

        while (rec != nullptr && sel(rec, Left))
            rec = idatfile.Next();
        //now rec == Left Border

        while (rec != nullptr) {
            if (sel(rec, Right))
                break; //rec > Right

            odatfile.Push(rec);
            rec = idatfile.Next();
        }
    }
};
template <class Tbase>
struct TSelectCore: public Tbase {
    static const size_t InputBuffer = 4 << 20;
    static const size_t OutputBuffer = 4 << 20;
    template <class TRec, class TSel>
    void operator()(const TRec* rec, TInDatFileImpl<TRec>& idatfile, TOutDatFileImpl<TRec>& odatfile, const TSel& sel) {
        typename TExtInfoType<TRec>::TResult extInfo;
        while ((rec = idatfile.Next()) != nullptr)
            if (sel(rec, (Tbase&)(*this))) {
                if (TExtInfoType<TRec>::Exists)
                    idatfile.GetExtInfo(&extInfo);
                odatfile.Push(rec, TExtInfoType<TRec>::Exists ? &extInfo : nullptr);
            }
    }
};
