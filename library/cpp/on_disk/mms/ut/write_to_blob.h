#pragma once

#include <library/cpp/on_disk/mms/cast.h>
#include <library/cpp/on_disk/mms/writer.h>

#include <util/generic/buffer.h>
#include <util/memory/blob.h>
#include <util/stream/buffer.h>

template <class T>
inline TBlob SafeWriteToBlob(const T& t) {
    TBufferOutput out;
    NMms::SafeWrite(out, t);
    out.Finish();
    return TBlob::FromBuffer(out.Buffer());
}

template <class T>
inline TBlob UnsafeWriteToBlob(const T& t) {
    TBufferOutput out;
    NMms::UnsafeWrite(out, t);
    out.Finish();
    return TBlob::FromBuffer(out.Buffer());
}

template <class T>
inline TBlob WriteToBlob(const T& t) {
    return SafeWriteToBlob(t);
}

template <class T>
const T& UnsafeCast(const TBlob& blob) {
    return NMms::UnsafeCast<T>(blob.AsCharPtr(), blob.Length());
}

template <class T>
const T& SafeCast(const TBlob& blob) {
    return NMms::SafeCast<T>(blob.AsCharPtr(), blob.Length());
}

template <class T>
const T& Cast(const TBlob& blob) {
    return UnsafeCast<T>(blob);
}
