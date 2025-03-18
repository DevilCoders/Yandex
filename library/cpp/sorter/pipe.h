#pragma once

#include <util/system/event.h>

template <class V>
class TMultPipeTmpl {
public:
    ui32 Size;
    ui32 N;
    ui32 CurWr;
    ui32 CurRd;
    ui32 PosWH, PosRH;
    std::vector<ui32> MaxRH;
    std::vector<bool> CanSwitch;
    std::vector<TAutoEvent> QEvent;
    std::vector<TAutoEvent> AEvent;
    ui32 WaitCount;
    ui32 SwitchCount;
    ui32 Guard;
    std::vector<bool> Ready;
    std::vector<std::vector<V>> HitsBuf;
    TString Name;
    bool First;

public:
    TMultPipeTmpl(int size, int n, const char* name = "")
        : Size(size)
        , N(n)
        , CurWr(0)
        , CurRd(0)
        , PosWH(0)
        , PosRH(0)
        , WaitCount(0)
        , SwitchCount(0)
        , Guard(0)
        , Name(name)
        , First(true)
    {
    }

    void Init() {
        MaxRH.resize(N);
        CanSwitch.resize(N);
        Ready.resize(N);
        HitsBuf.resize(N);
        for (ui32 i = 0; i < N; i++) {
            HitsBuf[i].resize(Size);
        }
        QEvent.resize(N);
        AEvent.resize(N);
        memset(&MaxRH[0], 0, N * sizeof(ui32));
        memset(&Ready[0], 0, N * sizeof(bool));
        memset(&CanSwitch[0], true, N * sizeof(bool));
        CanSwitch[0] = false;
        for (ui32 i = 1; i < N; i++)
            AEvent[i].Signal();
    }

    void Switch() {
        MaxRH[CurWr] = PosWH;
        PosWH = 0;
        Ready[CurWr] = true;
        ui32 curWr = CurWr;
        CurWr = (CurWr + 1) % N;
        SwitchCount++;
        QEvent[curWr].Signal();
        //        if (Size == BIG_PIPE_SIZE)
        //            fprintf(stderr, "QEvent[%i] signalled %016llx\n", curWr, (ui64)this);
        AEvent[CurWr].Wait();
        CanSwitch[CurWr] = false;
    }

    bool GetCanSwitch() {
        return CanSwitch[(CurWr + 1) % N];
    }

    void Put(V& val) {
        if (Y_UNLIKELY(PosWH >= Size)) {
            Switch();
        }
        HitsBuf[CurWr][PosWH] = val;
        PosWH++;
    }

    V& PutPtr() {
        if (Y_UNLIKELY(PosWH >= Size)) {
            Switch();
        }
        return HitsBuf[CurWr][PosWH++];
    }

    void Wait() {
        if (!First) {
            Ready[CurRd] = false;
            CanSwitch[CurRd] = true;
            AEvent[CurRd].Signal();
            CurRd = (CurRd + 1) % N;
        }
        //        do {
        //            fprintf(stderr, "Waiting %u wait count %u switch count: %u\n", CurRd, WaitCount, SwitchCount);
        //        } while (!QEvent[CurRd].Wait(100000));
        if (!Ready[CurRd])
            fprintf(stderr, "%s waiting %u wait count %u switch count: %u\n", Name.data(), CurRd, WaitCount, SwitchCount);
        QEvent[CurRd].Wait();
        WaitCount++;
        First = false;
        PosRH = 0;
    }

    ui32 BufAvail() {
        return PosRH < MaxRH[CurRd];
    }

    V* Alloc(ui32 asize) {
        if (asize > Size)
            throw yexception() << "To big object of size " << asize << " in pipe of size " << Size;
        if (Size - PosWH < asize)
            Switch();
        V* res = &HitsBuf[CurWr][PosWH];
        PosWH += asize;
        return res;
    }

    bool AllocWait(ui32 asize) {
        return (Size - PosWH < asize) && !GetCanSwitch();
    }

    ui32 GetBufs(V*& hits) {
        hits = &HitsBuf[CurRd][PosRH];
        return MaxRH[CurRd] - PosRH;
    }

    void Move(ui32 diff) {
        PosRH += diff;
        if (PosRH >= MaxRH[CurRd])
            Ready[CurRd] = false;
    }

    V Get() {
        if (Y_UNLIKELY(First || (PosRH >= MaxRH[CurRd])))
            Wait();
        return HitsBuf[CurRd][PosRH++];
    }

    V* GetPtr() {
        if (Y_UNLIKELY(First || (PosRH >= MaxRH[CurRd])))
            Wait();
        return &HitsBuf[CurRd][PosRH++];
    }

    void Flush() {
        Ready[CurWr] = true;
        MaxRH[CurWr] = PosWH;
        QEvent[CurWr].Signal();
    }

    bool GetReady() {
        return Ready[(CurRd + 1) % N];
    }
};
