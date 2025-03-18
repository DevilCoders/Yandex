#pragma once

#include <util/generic/yexception.h>
#include <util/generic/vector.h>
#include <util/stream/input.h>

namespace NMemstatsParser {
typedef TVector<ui64> TBackTrace;

struct TItem {
    explicit TItem() :
        Delete(), Ptr(), Len(), Hndl() {
    }

    explicit TItem(ui64 ptr, ui64 hndl) :
        Delete(true), Ptr(ptr), Len(), Hndl(hndl) {
    }

    explicit TItem(ui64 ptr, ui64 len, ui64 hndl) :
        Delete(), Ptr(ptr), Len(len), Hndl(hndl) {
    }

    bool Delete;
    ui64 Ptr;
    ui64 Len;
    ui64 Hndl;
    TBackTrace Bt;
};

inline TItem ReadAlloc(IInputStream& in, bool havebt = false) {
    ui64 ptr;
    ui32 len;
    ui64 hnd;

    if (in.Load(&ptr, sizeof(ptr)) != sizeof(ptr))
        ythrow yexception () << "protocol error";

    if (in.Load(&len, sizeof(len)) != sizeof(len))
        ythrow yexception () << "protocol error";

    if (in.Load(&hnd, sizeof(hnd)) != sizeof(hnd))
        ythrow yexception () << "protocol error";

    TItem item(ptr, len, hnd);

    if (havebt) {
        unsigned char cnt = 0;

        if (!in.Read(&cnt, 1))
            ythrow yexception () << "protocol error";

        TBackTrace& bt = item.Bt;

        for (; cnt; --cnt) {
            ui64 ptr;

            if (in.Load(&ptr, sizeof(ptr)) != sizeof(ptr))
                ythrow yexception () << "protocol error";

            bt.push_back(ptr);
        }
    }

    return item;
}

inline TItem ReadFree(IInputStream& in) {
    ui64 ptr;
    ui64 hnd;

    if (in.Load(&ptr, sizeof(ptr)) != sizeof(ptr))
        ythrow yexception () << "protocol error";

    if (in.Load(&hnd, sizeof(hnd)) != sizeof(hnd))
        ythrow yexception () << "protocol error";

    return TItem(ptr, hnd);
}

bool ReadItem(TItem& item, IInputStream& in) {
    ui8 type;

    if (!in.Read(&type, 1))
        return false;

    switch (type) {
    case 0xA0:
        item = ReadAlloc(in, true);
        break;
    case 0xA1:
        item = ReadAlloc(in);
        break;
    case 0xA2:
        item = ReadFree(in);
        break;
    default:
        ythrow yexception () << "protocol error";
    }

    return true;
}
}
