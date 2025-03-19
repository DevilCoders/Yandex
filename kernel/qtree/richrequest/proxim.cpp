#include "proxim.h"
#include <util/generic/map.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>

TProximity::TProximity(int beg, int end, int level, TDistanceType disttype /*= DT_SYNTAX*/)
    : DistanceType(disttype)
{
    Y_ASSERT(level == 0 || level == 1 || level == 2);
    if (beg || end) {
        Beg = beg;
        End = end;
        Level = level ? DOC_LEVEL : BREAK_LEVEL;
        Center = (Level == BREAK_LEVEL) ? ((Beg + End) >> 1) : 1;
    } else if (level) {
        Level = level != 1 ? DOC_LEVEL : BREAK_LEVEL;
        Shift = (Level == BREAK_LEVEL) ? BREAK_LEVEL_Shift : DOC_LEVEL_Shift;
        ShiftLow = (Level == BREAK_LEVEL) ? WORD_LEVEL_Shift : BREAK_LEVEL_Shift;
        Beg = -((1L << Shift) >> ShiftLow);
        End =  ((1L << Shift) >> ShiftLow);
        Center = 1;
    } else {
        Beg = 0;
        End = 0;
        Level = BREAK_LEVEL;
        Center = 0;
    }
    Init();
}
