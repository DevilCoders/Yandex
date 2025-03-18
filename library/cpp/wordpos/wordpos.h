#pragma once

#include <util/system/defaults.h>
#include <util/system/yassert.h>

// chapId
// [1..MAX_CHAP]

// ---------- usual word -----------
// docId       .paraId       .sentId       .wordId       .weight
// [0..MAX_DOC].[1..MAX_PARA].[1..MAX_SENT].[1..MAX_WORD].[0..BEST_WEIGHT]

//----------- titles ---------------
// a) using # 0
// DOCUMENT TITLE
//    docId       .0.sentId       .wordId       .weight
//    [0..MAX_DOC].0.[1..MAX_SENT].[1..MAX_WORD].[0..BEST_WEIGHT]
// SUB-UNIT TITLE (has meaning for chapters)
//    docId       .paraId       .0  .wordId     .weight
//    [0..MAX_DOC].[1..MAX_PARA].0.[1..MAX_WORD].[0..BEST_WEIGHT]

// b) using best weight
//    docId       .paraId       .sentId       .wordId       .BEST_WEIGHT
//    [0..MAX_DOC].[1..MAX_PARA].[1..MAX_SENT].[1..MAX_WORD].BEST_WEIGHT

//----------- doc attributes -----
//    docId       .0.0.0.LOW_WEIGHT
//    [0..MAX_DOC].0.0.0.LOW_WEIGHT

// ?
// ---------- doc reference -----------
//  doc          #  paraId        #  sentId         #  wordId
// [0..MAX_DOC]  # [0]            # [0]             # [0]

// ---------- of sub-unit reference ---
//  doc          #  paraId        #  sentId         #  wordId
// [0..MAX_DOC]  # [1..MAX_PARA]  # [0]             # [0]

#define INT_N_MAX(nbits) (((i32)1 << (nbits)) - 1)
//#define USEFUL_WIDTH_MAX              (64-7)

const int NFORM_LEVEL_Bits = 4;

const int RELEV_LEVEL_Bits = 2;
//#define RELEV_LEVEL_Behaviour      OverflowIgnore

const int WORD_LEVEL_Bits = 6;
#define WORD_LEVEL_Behaviour OverflowStartNew

const int BREAK_LEVEL_Bits = 15;
#define BREAK_LEVEL_Behaviour (OverflowReport | UnderflowIgnore)

const int DOC_LEVEL_Bits = 26;
//#define DOC_LEVEL_Behaviour        OverflowIgnore

const int NFORM_LEVEL_Shift = 0;
const unsigned NFORM_LEVEL_Max = INT_N_MAX(NFORM_LEVEL_Bits);
const SUPERLONG NFORM_LEVEL_Mask = (((SUPERLONG)NFORM_LEVEL_Max) << NFORM_LEVEL_Shift);

const int RELEV_LEVEL_Shift = (NFORM_LEVEL_Shift + NFORM_LEVEL_Bits);
const unsigned RELEV_LEVEL_Max = INT_N_MAX(RELEV_LEVEL_Bits);
const SUPERLONG RELEV_LEVEL_Mask = (((SUPERLONG)RELEV_LEVEL_Max) << RELEV_LEVEL_Shift);

const int WORD_LEVEL_Shift = (RELEV_LEVEL_Shift + RELEV_LEVEL_Bits);
const unsigned WORD_LEVEL_Max = INT_N_MAX(WORD_LEVEL_Bits);
const SUPERLONG WORD_LEVEL_Mask = (((SUPERLONG)WORD_LEVEL_Max) << WORD_LEVEL_Shift);

const int BREAK_LEVEL_Shift = (WORD_LEVEL_Shift + WORD_LEVEL_Bits);
const unsigned BREAK_LEVEL_Max = INT_N_MAX(BREAK_LEVEL_Bits);
const SUPERLONG BREAK_LEVEL_Mask = (((SUPERLONG)BREAK_LEVEL_Max) << BREAK_LEVEL_Shift);

// + 1 to get on dword border
const int DOC_LEVEL_Shift = (BREAK_LEVEL_Shift + BREAK_LEVEL_Bits);
const unsigned DOC_LEVEL_Max = INT_N_MAX(DOC_LEVEL_Bits);
const SUPERLONG DOC_LEVEL_Mask = (((SUPERLONG)DOC_LEVEL_Max) << DOC_LEVEL_Shift);

const SUPERLONG BREAKWORDRELEV_LEVEL_Mask = BREAK_LEVEL_Mask | WORD_LEVEL_Mask | RELEV_LEVEL_Mask;
const SUPERLONG DOCBREAKWORD_LEVEL_Mask = DOC_LEVEL_Mask | BREAK_LEVEL_Mask | WORD_LEVEL_Mask;
const SUPERLONG DOCBREAK_LEVEL_Mask = DOC_LEVEL_Mask | BREAK_LEVEL_Mask;

const unsigned POSTING_MAX = INT_N_MAX(DOC_LEVEL_Shift);
const unsigned POSTING_ERR = 1 << DOC_LEVEL_Shift;

const unsigned N_MAX_FORMS_PER_KISHKA = (NFORM_LEVEL_Max + 1);

enum RelevLevel {
    NOINDEX_RELEV = -1, //-- spam
    LOW_RELEV = 0,      //-- <pre> and so on
    ANCHOR_RELEV = 0,   //--- text in tag <a>, see BUKI-1288
    MID_RELEV = 1,      //-- normal text
    HIGH_RELEV = 2,     //-- headings
    BEST_RELEV = 3,     //-- title, word in text from meta.keywords(maybe in prev. level?)
    NUM_RELEVS = 4,
};

enum WORDPOS_LEVEL {
    NFORM_LEVEL = 0,
    RELEV_LEVEL = 1,
    WORD_LEVEL = 2,
    BREAK_LEVEL = 3,
    DOC_LEVEL = 4,
};

using TPosting = ui32;

struct TWordPosition {
    enum {
        OverflowStartNew = 1,
        OverflowReport = 2,
        OverflowIgnore = 4,

        UnderflowIgnore = 8,
    };

    enum {
        FIRST_CHILD = 1,
    };

    static Y_FORCE_INLINE ui32 Doc(const SUPERLONG& x) {
        return (ui32)((ui64)x >> DOC_LEVEL_Shift); // & DOC_LEVEL_Max;
    }

    template <class T>
    static Y_FORCE_INLINE ui32 Break(const T& x) {
        return ((ui32)x >> BREAK_LEVEL_Shift) & BREAK_LEVEL_Max;
    }
    template <class T>
    static Y_FORCE_INLINE ui32 BreakAndWord(const T& x) {
        return (((ui32)x & BREAKWORDRELEV_LEVEL_Mask) >> WORD_LEVEL_Shift);
    }
    template <class T>
    static Y_FORCE_INLINE ui32 Word(const T& x) {
        return ((ui32)x >> WORD_LEVEL_Shift) & WORD_LEVEL_Max;
    }
    template <class T>
    static Y_FORCE_INLINE RelevLevel GetRelevLevel(const T& x) {
        return (RelevLevel)(((ui32)x >> RELEV_LEVEL_Shift) & RELEV_LEVEL_Max);
    }

    template <class T>
    static Y_FORCE_INLINE ui32 Form(const T& x) {
        return ((ui32)x >> NFORM_LEVEL_Shift) & NFORM_LEVEL_Max;
    }

    static ui32 DocLength(const SUPERLONG& x) {
        Y_ASSERT(INT_N_MAX(DOC_LEVEL_Shift) <= 0x7FFFFFFF); // assume unsigned operation
        return (ui32)(x & INT_N_MAX(DOC_LEVEL_Shift));
    }

    static SUPERLONG PositionOnly(SUPERLONG x) {
        return x &= ~(SUPERLONG(RELEV_LEVEL_Max << RELEV_LEVEL_Shift) | NFORM_LEVEL_Max);
    }

    static SUPERLONG CurrentBreakPos(SUPERLONG x) {
        return x & ~((SUPERLONG(1) << BREAK_LEVEL_Shift) - 1);
    }

    static SUPERLONG NextBreakPos(SUPERLONG x) {
        return CurrentBreakPos(x) + (SUPERLONG(1) << BREAK_LEVEL_Shift);
    }

    static void Set(SUPERLONG& pos, ui32 high, ui32 medium,
                    ui32 low, RelevLevel relLev = LOW_RELEV, i32 nForm = 0) {
        Y_ASSERT(relLev != NOINDEX_RELEV);
        ((((((((pos = (high & DOC_LEVEL_Max)) <<= BREAK_LEVEL_Bits) |= (medium & BREAK_LEVEL_Max)) <<= WORD_LEVEL_Bits) |= (low & WORD_LEVEL_Max)) <<= RELEV_LEVEL_Bits) |= (relLev & RELEV_LEVEL_Max)) <<= NFORM_LEVEL_Bits) |= (nForm & NFORM_LEVEL_Max);
    }

    static Y_FORCE_INLINE void SetDoc(SUPERLONG& pos, ui32 doc) {
        (pos &= ~(i64(DOC_LEVEL_Max) << DOC_LEVEL_Shift)) |= i64(doc & DOC_LEVEL_Max) << DOC_LEVEL_Shift;
    }
    static Y_FORCE_INLINE void SetBreak(SUPERLONG& pos, ui32 brk) {
        (pos &= ~(i64)(BREAK_LEVEL_Max << BREAK_LEVEL_Shift)) |= (brk & BREAK_LEVEL_Max) << BREAK_LEVEL_Shift;
    }
    static Y_FORCE_INLINE void SetWord(SUPERLONG& pos, ui32 wrd) {
        (pos &= ~(i64)(WORD_LEVEL_Max << WORD_LEVEL_Shift)) |= (wrd & WORD_LEVEL_Max) << WORD_LEVEL_Shift;
    }
    static Y_FORCE_INLINE void SetRelevLevel(SUPERLONG& pos, int relevLevel) {
        Y_ASSERT(relevLevel != NOINDEX_RELEV);
        (pos &= ~(i64)(RELEV_LEVEL_Max << RELEV_LEVEL_Shift)) |= (relevLevel & RELEV_LEVEL_Max) << RELEV_LEVEL_Shift;
    }
    static Y_FORCE_INLINE void SetWordForm(SUPERLONG& pos, unsigned nForm) {
        Y_ASSERT(nForm <= NFORM_LEVEL_Max);
        (pos &= ~(i64)(NFORM_LEVEL_Max << NFORM_LEVEL_Shift)) |= (nForm & NFORM_LEVEL_Max) << NFORM_LEVEL_Shift;
    }

    static Y_FORCE_INLINE void SetBreak(i32& pos, ui32 brk) {
        (pos &= ~(i32)(BREAK_LEVEL_Max << BREAK_LEVEL_Shift)) |= (brk & BREAK_LEVEL_Max) << BREAK_LEVEL_Shift;
    }
    static Y_FORCE_INLINE void SetWord(i32& pos, ui32 wrd) {
        (pos &= ~(i32)(WORD_LEVEL_Max << WORD_LEVEL_Shift)) |= (wrd & WORD_LEVEL_Max) << WORD_LEVEL_Shift;
    }
    static Y_FORCE_INLINE void SetRelevLevel(i32& pos, int relevLevel) {
        Y_ASSERT(relevLevel != NOINDEX_RELEV);
        (pos &= ~(i32)(RELEV_LEVEL_Max << RELEV_LEVEL_Shift)) |= (relevLevel & RELEV_LEVEL_Max) << RELEV_LEVEL_Shift;
    }
    static Y_FORCE_INLINE void SetWordForm(i32& pos, unsigned nForm) {
        Y_ASSERT(nForm <= NFORM_LEVEL_Max);
        (pos &= ~(i32)(NFORM_LEVEL_Max << NFORM_LEVEL_Shift)) |= (nForm & NFORM_LEVEL_Max) << NFORM_LEVEL_Shift;
    }

    SUPERLONG Pos;

    TWordPosition() = default;

    TWordPosition(ui32 high, ui32 medium, ui32 low, RelevLevel relLev = LOW_RELEV, i32 nForm = 0) {
        Set(high, medium, low, relLev, nForm);
    }

    TWordPosition(SUPERLONG x)
        : Pos(x)
    {
    }

    // for statistics (I would say for docLength? looks like it is used everywhere)
    TWordPosition(ui32 nDocument, ui32 medium_and_low_and_relev) {
        Pos = ((SUPERLONG)(nDocument & DOC_LEVEL_Max)) << DOC_LEVEL_Shift;
        Pos |= medium_and_low_and_relev & INT_N_MAX(DOC_LEVEL_Shift);
    }

    SUPERLONG SuperLong() const {
        return Pos;
    }

    void Set(ui32 high, ui32 medium, ui32 low, RelevLevel relLev = LOW_RELEV, i32 nForm = 0) {
        Set(Pos, high, medium, low, relLev, nForm);
    }

    void SetDoc(ui32 doc) {
        SetDoc(Pos, doc);
    }
    void SetBreak(ui32 brk) {
        SetBreak(Pos, brk);
    }
    void SetWord(ui32 wrd) {
        SetWord(Pos, wrd);
    }
    void SetRelevLevel(int relevLevel) {
        SetRelevLevel(Pos, relevLevel);
    }
    void SetWordForm(int nForm) {
        SetWordForm(Pos, nForm);
    }

    bool Inc() // returns 1 if successfully incremented
    {
        if (Word() < WORD_LEVEL_Max) {
            SetWord(Word() + 1);
        } else if (!Bump()) {
            return false;
        }

        return true;
    }

    bool Bump() {
        // skip empty ?
        if (Word() <= FIRST_CHILD)
            return true;

        if (Break() < BREAK_LEVEL_Max) {
            SetBreak(Break() + 1);
        } else {
            return false;
        }

        SetWord(FIRST_CHILD);
        return true;
    }

    ui32 Doc() const {
        return Doc(Pos);
    }
    ui16 Break() const {
        return (ui16)Break(Pos);
    }
    ui32 Word() const {
        return Word(Pos);
    }
    RelevLevel GetRelevLevel() const {
        return GetRelevLevel(Pos);
    }
    ui32 Form() const {
        return Form(Pos);
    }
    ui32 DocLength() const {
        return DocLength(Pos);
    }

    // booleans
    bool InTitle() const {
        return GetRelevLevel() == BEST_RELEV; // can be (Break() == 0)
    }
    bool IsDocAttr() const {
        return Break() == 0;
    }
};

inline void SetPosting(TPosting& Pos, ui32 brk, ui32 wrd, RelevLevel relLev = LOW_RELEV) {
    Y_ASSERT(relLev != NOINDEX_RELEV);
    Pos = 0;
    ((((((Pos |= (brk & BREAK_LEVEL_Max)) <<= WORD_LEVEL_Bits) |= (wrd & WORD_LEVEL_Max)) <<= RELEV_LEVEL_Bits) |= (relLev & RELEV_LEVEL_Max)) <<= NFORM_LEVEL_Bits) |= (0 & NFORM_LEVEL_Max);
}

inline void SetPostingRelevLevel(TPosting& Pos, RelevLevel relevLevel) {
    Y_ASSERT(relevLevel != NOINDEX_RELEV);
    Pos &= ~(i64)(RELEV_LEVEL_Max << RELEV_LEVEL_Shift);
    Pos |= (relevLevel & RELEV_LEVEL_Max) << RELEV_LEVEL_Shift;
}

inline RelevLevel GetRelevLevel(TPosting Pos) {
    return (RelevLevel)((Pos >> RELEV_LEVEL_Shift) & RELEV_LEVEL_Max);
}

inline void SetPostingNForm(TPosting* pRes, ui32 nForm) {
    Y_ASSERT(nForm <= NFORM_LEVEL_Max);
    *pRes &= ~i64(NFORM_LEVEL_Max << NFORM_LEVEL_Shift);
    *pRes |= (nForm & NFORM_LEVEL_Max) << NFORM_LEVEL_Shift;
}

inline int GetPostingNForm(TPosting n) {
    return (n & (NFORM_LEVEL_Max << NFORM_LEVEL_Shift)) >> NFORM_LEVEL_Shift;
}

inline ui16 GetBreak(TPosting n) {
    return ui16((n >> BREAK_LEVEL_Shift) & BREAK_LEVEL_Max);
}

inline ui16 GetWord(TPosting n) {
    return ui16((n >> WORD_LEVEL_Shift) & WORD_LEVEL_Max);
}

//! increments position, it can be shifted to the next sentence
inline TPosting PostingInc(TPosting pos) {
    if ((pos >> DOC_LEVEL_Shift) != 0) /* Is attribute posting? */
        return pos;
    TWordPosition wp((SUPERLONG)pos);
    wp.Inc();
    return (TPosting)wp.SuperLong();
}

//! shifts position for the specified offset inside a sentence
//! @attention position must not go out of the current sentence
inline TPosting PostingInc(TPosting pos, ui8 offset) {
    if (offset == 0)
        return pos;
    if ((pos >> DOC_LEVEL_Shift) != 0) /* Is attribute posting? */
        return pos;
    TWordPosition wp((SUPERLONG)pos);
    const ui32 word = wp.Word();
    Y_ASSERT(word + offset <= WORD_LEVEL_Max); // it must be controlled in the numerator
    wp.SetWord(word + offset);
    return (TPosting)wp.SuperLong();
}

#define START_POINT -1L

enum EFormClass {
    EQUAL_BY_STRING = 0,
    EQUAL_BY_LEMMA = 1,
    EQUAL_BY_SYNONYM = 2,
    EQUAL_BY_SYNSET = 3,

    NUM_OLD_FORM_CLASSES = 3, // compatibility with existing factors
    NUM_FORM_CLASSES = 4      // == SKIP
};

#define FINAL_DOC_BITS ((1ll << 53) - 1)

Y_FORCE_INLINE SUPERLONG GetDocBits(SUPERLONG pos) {
    return pos & DOC_LEVEL_Mask;
}

Y_FORCE_INLINE SUPERLONG GetPositionBits(SUPERLONG pos) {
    return pos & DOCBREAKWORD_LEVEL_Mask;
}
