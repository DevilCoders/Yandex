#pragma once

#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/utility.h>

struct TRTGroundData;
struct TRTGroundDataSet;

// Encodes hits in 16-aligned blocks.
class TRTCoder {
private:
    const TRTGroundDataSet*const DecoderData;
public:
    TRTCoder() noexcept;
    ~TRTCoder() {
    }

    // Writes 16 bytes during a single call. Always increases dst by 16 bytes. count must be positive!
    ui64* Output(const SUPERLONG*& hits, ui64*__restrict dst, size_t& count) const noexcept;
    // Writes several 16 bytes blocks. Increases dst by 16 * number-of-blocks-written.
    ui64* OutputAll(const SUPERLONG* hits, ui64*__restrict dst, size_t count) const noexcept;
};

// Decodes hits in 16-aligned blocks.
class TRTDecoder {
private:
    const ui64*__restrict Start;
    const ui64*__restrict End;
    ui64 Count;
    SUPERLONG CurrentHit;
    ui64 Data;
    const TRTGroundData*__restrict GroundDataPtr;
    const TRTGroundDataSet*const DecoderData;

public:
    static const SUPERLONG MaxValue = (1ULL << 63) - 1;

    TRTDecoder() noexcept;
    ~TRTDecoder() {
    }

    void Set(const ui64* start, const ui64* end) noexcept {
        Start = start;
        End = end;
        Count = 0;
        CurrentHit = START_POINT;
        Data = 0;
        GroundDataPtr = nullptr;
    }
    size_t GetLength() const noexcept {
        return (End - Start) * sizeof(ui64);
    }
    bool IsValid() const noexcept {
        return Start < End;
    }

    void Next() noexcept;

    void SkipToOrBreakDown(SUPERLONG to) noexcept;
    void SkipCount(size_t count) noexcept;
    void SkipAndCount(SUPERLONG to, i64& count) noexcept;

    Y_FORCE_INLINE SUPERLONG GetCurrent() const noexcept {
        return CurrentHit;
    }
};
