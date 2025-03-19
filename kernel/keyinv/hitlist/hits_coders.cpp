#include <util/generic/utility.h>
#include <util/generic/algorithm.h>
#include <util/system/yassert.h>

#include "hits_coders.h"
#include <kernel/keyinv/hitlist/inv_code/inv_code.h>

int CHitGroupCoder::Flush(char *dst) {
    if (Ptr == 1)
        return 0;
    for (size_t i = Ptr; i < 9; ++i)
        Data[i] = Data[i - 1];

    char *res = (char *)Compress(Data + 1, (ui8 *)dst, (ui8)(Ptr - 1));
    Data[0] = Data[8];
    Ptr = 1;
    return int(res - dst);
}

bool CHitGroupCoder::Output(SUPERLONG l) {
    Data[Ptr] = l;
    ++Ptr;
    return (Ptr == 9); // if true flush must be called
}

int CHitCoder::Finish(char* dst) {
    if (HitFormat  == HIT_FMT_BLK8) {
        const SUPERLONG* const data = GroupCoder.GetData();
        const size_t ptr = GroupCoder.GetPtr();
        assert(ptr < 9);
        if (data[0] == START_POINT) { // fallback
            int len = DumpHeader(dst);
            for (size_t i = 1; i < ptr; ++i)
                len += CodeWordPos(data[i - 1], data[i], dst + len);
            return len;
        } else {
            const int n = GroupCoder.Flush(dst);
            return n + DumpJunk(dst + n);
        }
    }
    return 0;
}

int CHitCoder::FinishNoJunk(char* dst, bool& dj) {
    if (HitFormat  == HIT_FMT_BLK8) {
        const SUPERLONG* const data = GroupCoder.GetData();
        const size_t ptr = GroupCoder.GetPtr();
        Y_ASSERT(ptr < 9);
        if (data[0] == START_POINT) { // fallback
            int len = DumpHeader(dst);
            for (size_t i = 1; i < ptr; ++i)
                len += CodeWordPos(data[i - 1], data[i], dst + len);
            dj = false;
            return len;
        } else {
            dj = true;
            return GroupCoder.Flush(dst);
        }
    }
    return 0;
}

int CHitCoder::Output(SUPERLONG l, char *dst) {
    Y_ASSERT(l >= Current); // not required, but violation of this most probably is a bug
    SUPERLONG newCurrent = l;
    int format = HitFormat;
    if (format  == HIT_FMT_BLK8) {
        if (l >= Current) {
            if (GroupCoder.Output(Current = l)) {
                const SUPERLONG* const data = GroupCoder.GetData();
                if (data[0] == START_POINT) {
                    const int n = DumpHeader(dst);
                    return n + GroupCoder.Flush(dst + n);
                }
                return GroupCoder.Flush(dst);
            }
        }
        return 0;
    } else if (format == HIT_FMT_RAW_I64) {
        ;
    } else if (format == HIT_FMT_V2) {
        int nRes = CodeWordPos(Current, l, dst);
        Current = l;
        return nRes;
    } else {
        printf("%d \n", format);
        assert(0);
    }
    SUPERLONG diff = l - Current;
    Current = newCurrent;
    return out_long(diff, dst);
}

void CHitCoder::Swap(CHitCoder& rhs) {
    DoSwap(Current, rhs.Current);
    std::swap(HitFormat, rhs.HitFormat);
}

void CHitDecoder::SetCurrent(SUPERLONG l) {
    Current = l;
    CurrentInternal = l;
}

void CHitDecoder::SetCurrentNative(SUPERLONG l) {
    Y_ASSERT(HitFormat == HIT_FMT_RAW_I64 || HitFormat == HIT_FMT_V2);
    CurrentInternal = l;
    Current = CurrentInternal;
}

void CHitDecoder::Next(const char *&data) {
    if (Y_LIKELY(HitFormat  == HIT_FMT_V2)) {
        data = DecodeWordPos(CurrentInternal, &Current, data);
        CurrentInternal = Current;
        return;
    }
    ReadNext(data);
    Y_ASSERT(HitFormat == HIT_FMT_RAW_I64);
    Current = CurrentInternal;
}

bool CHitDecoder::SkipTo(const char *&Cur0, const char *Upper, SUPERLONG to) {
    if (Y_LIKELY(HitFormat == HIT_FMT_V2)) {
        bool bRes = true;
        while (CurrentInternal < to) {
            if (Cur0 < Upper) { // actually need to check only at the last position
                Cur0 = DecodeWordPos(CurrentInternal, &Current, Cur0);
                CurrentInternal = Current;
            } else {
                bRes = false;
                break;
            }
        }
        return bRes;
    }
    Y_ASSERT(HitFormat == HIT_FMT_RAW_I64);
    bool bRes = true;
    while (CurrentInternal < to) {
        if (Cur0 < Upper) { // actually need to check only at the last position
            ReadNext(Cur0);
        } else {
            bRes = false;
            break;
        }
    }
    Current = CurrentInternal;
    return bRes;
}

/////////////////////////////////////////////////////////////

void CHitDecoder2::Next(const char *&data) {
    data = DecodeWordPos(Current, &Current, data);
}

bool CHitDecoder2::SkipTo(const char*& cur0, const char* upper, SUPERLONG to) {
    SUPERLONG current = Current;
    const char *ptr = cur0;
    while (current < to) {
        if (ptr < upper) { // actually need to check only at the last position
            ptr = DecodeWordPos(current, &current, ptr);
        } else {
            return false;
        }
    }
    Current = current;
    cur0 = ptr;
    return true;
}
