#pragma once

#include <kernel/qtree/richrequest/protos/proxim.pb.h>

#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/fwd.h>
#include <util/generic/utility.h>

#include <algorithm>
#include <cstdlib>

#define UNDEF_CENTER INT_MAX   //-- bigger than number of words in break

                      /* diff =   0, 1, 2, 3, 4, 5, 6, 7, 8 */
const int DIST_WEIGHT_COEFF[] = {16, 8, 4, 4, 4, 2, 2, 2, 2};

struct TOpInfo;

//Расстояние между поддеревьями, принадлежащими вершине.
enum TDistanceType {
    DT_NONE = 0,
    DT_MULTITOKEN, // мультитокен
    DT_QUOTE,      // последовательность слов или слово в кавычках
    DT_PHRASE,     // последовательность слов, разделенных пробелами
    DT_USEROP,     // последовательность слов, разделенных расстояниями, указанными пользователем - &&, &, /(1,1), ~~  и т.п.
    DT_SYNTAX      // "generic" syntactic distance
};

const TString& ToString(TDistanceType);
bool FromString(const TString& name, TDistanceType& ret);

inline bool IsNeighbour(TDistanceType dt) {
    return dt == DT_MULTITOKEN || dt == DT_QUOTE || dt == DT_USEROP;
}

struct IInfoDataTable;

class TProximity {
    friend void PrintProximity(const TProximity& proximity, IInfoDataTable* table);
private:
    unsigned Shift;
    unsigned ShiftLow;
    i32 LevelMask;
    i32 LevelMaskLow;
    i32 LevelIncr;
    i32 ShiftedBeg;
    i32 ShiftedEnd;
public:
    TDistanceType DistanceType;
    WORDPOS_LEVEL Level;
    int Beg, End; // in words or breaks depending on Level
    int Center; // in words always

public:
    void ToProto(NProximity::TProximity& pr) const {
        pr.SetShift(Shift);
        pr.SetShiftLow(ShiftLow);
        pr.SetLevelMask(LevelMask);
        pr.SetLevelMaskLow(LevelMaskLow);
        pr.SetLevelIncr(LevelIncr);
        pr.SetShiftedBeg(ShiftedBeg);
        pr.SetShiftedEnd(ShiftedEnd);
        pr.SetBeg(Beg);
        pr.SetEnd(End);
        pr.SetCenter(Center);
        pr.SetDistanceType(DistanceType);
        pr.SetLevel(Level);
    }

    void FromProto(const NProximity::TProximity& pr) {
        Shift = pr.GetShift();
        ShiftLow = pr.GetShiftLow();
        LevelMask = pr.GetLevelMask();
        LevelMaskLow = pr.GetLevelMaskLow();
        LevelIncr = pr.GetLevelIncr();
        ShiftedBeg = pr.GetShiftedBeg();
        ShiftedEnd = pr.GetShiftedEnd();
        Beg = pr.GetBeg();
        End = pr.GetEnd();
        Center = pr.GetCenter();
        DistanceType = static_cast<TDistanceType>(pr.GetDistanceType());
        Level = static_cast<WORDPOS_LEVEL>(pr.GetLevel());
    }

    // Initialize additional stuff _after_ all semantic fields have got their values
    void Init() {
        Shift = Level == BREAK_LEVEL ? BREAK_LEVEL_Shift : DOC_LEVEL_Shift;
        ShiftLow = Level == BREAK_LEVEL ? WORD_LEVEL_Shift : BREAK_LEVEL_Shift;
        LevelIncr = i32(1) << Shift;
        i32 mask = LevelIncr - 1;
        LevelMask = ~mask;
        LevelMaskLow = ~((i32(1) << ShiftLow) - 1);
        ShiftedBeg = (i32)Min(Max(i64(static_cast<ui64>(Beg) << ShiftLow), -(i64)mask), (i64)mask);
        ShiftedEnd = (i32)Min(Max(i64(static_cast<ui64>(End) << ShiftLow), -(i64)mask), (i64)mask);
    }

    TProximity(int beg, int end, WORDPOS_LEVEL lev = BREAK_LEVEL, int center = UNDEF_CENTER)
        : DistanceType(DT_NONE)
        , Level(lev)
        , Beg(beg)
        , End(end)
        , Center(center != UNDEF_CENTER ? center
            : Level == DOC_LEVEL || !(Beg || End) ? 1
            : (Beg + End) / 2)
    {
        Init();
    }
    explicit TProximity(WORDPOS_LEVEL lev = BREAK_LEVEL)
        : DistanceType(DT_NONE)
        , Level(lev)
        , Center(1)
    {
        Shift = Level == BREAK_LEVEL ? BREAK_LEVEL_Shift : DOC_LEVEL_Shift;
        ShiftLow = Level == BREAK_LEVEL ? WORD_LEVEL_Shift : BREAK_LEVEL_Shift;
        End = ((1 << Shift) >> ShiftLow);
        Beg = -End;
        Init();
    }
    TProximity(int beg, int end, int level, TDistanceType disttype = DT_SYNTAX);

    bool operator==(const TProximity &pr) const {
        return Beg == pr.Beg && End == pr.End && Level == pr.Level && Center == pr.Center && DistanceType == pr.DistanceType;
    }
    TProximity operator-() const {
        return TProximity(-End, -Beg, Level, -Center);
    }
    bool IsContact(i32 e2, i32 e1) const {
         if ((e2 ^ e1) & LevelMask)
            return false;
         i32 p2 = (e2 & LevelMaskLow);
         i32 p1 = (e1 & LevelMaskLow);
         return (p2 >= ShiftedBeg + p1) && (p2 <= ShiftedEnd + p1);
    }
    int GetDistance(i32 e1, i32 e2) const {
        //-- PRECONDITION e1 & e2 in same document !
        if ((e2 ^ e1) & ~(i32)INT_N_MAX(BREAK_LEVEL_Shift)) // (e2 >> BREAK_LEVEL_Shift) != (e1 >> BREAK_LEVEL_Shift)
            //-- different pagraphs
            return 64;
        int diff = int((e2 >> WORD_LEVEL_Shift) - (e1 >> WORD_LEVEL_Shift));
        diff = diff > Center ? diff - Center : Center - diff;
        return diff;
    }
    static int DistWeight(int diff) {
        Y_ASSERT(diff >= 0);
        return diff > 8 ? 1 : DIST_WEIGHT_COEFF[diff];
    }
    int MaxCount() const {
        return 1 << Shift;
    }
    void GetBounds(i32 c, i32 &low, i32 &high) const {
        i32 cLevelBeg = c & LevelMask;
        i32 cLevelBegIncr = cLevelBeg + LevelIncr - 1;
        low =  (c & ~INT_N_MAX(ShiftLow)) + ShiftedBeg;
        if (low < cLevelBeg)
            low = cLevelBeg;
        if (low > cLevelBegIncr)
            low = cLevelBegIncr;
        high = (c | INT_N_MAX(ShiftLow)) + ShiftedEnd;
        if (high < cLevelBeg)
            high = cLevelBeg;
        if (high > cLevelBegIncr)
            high = cLevelBegIncr;
    }
    TProximity operator +(const TProximity &p2) const {
        if (Level > p2.Level)
            return *this;
        if (p2.Level > Level)
            return p2;
        int c1 = Center + p2.Center;
        int c2 = (End + Beg + p2.End + p2.Beg) / 2;
        return TProximity(Beg + p2.Beg, End + p2.End, Level, abs(c1) > abs(c2) ? c1 : c2);
    }
    SUPERLONG GetCenteringOffset() const {
        Y_ASSERT(ShiftedEnd >= ShiftedBeg);
        if (ShiftedBeg > 0)
            return -ShiftedBeg;
        if (ShiftedEnd < 0)
            return -ShiftedEnd;
        return 0;
    }
    void SetDistance(int beg, int end, WORDPOS_LEVEL lev, TDistanceType dist) {
        Beg = beg;
        End = end;
        Level = lev;
        DistanceType = dist;
    }
};
