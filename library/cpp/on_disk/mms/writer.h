#pragma once

#include <contrib/libs/mms/impl/defs.h>
#include <contrib/libs/mms/writer.h>

#include <util/stream/output.h>
#include <util/system/sanitizers.h>

namespace NMms {
    class TWriter: public mms::impl::WriterBase {
    public:
        explicit TWriter(IOutputStream& out)
            : Out(&out)
        {
        }

        void write(const void* data, size_t size) {
            NSan::Unpoison(data, size);

            Out->Write(data, size);
            advance(size);
        }

    private:
        IOutputStream* Out;
    };

    /// Writes element (with its format version) to a stream.
    template <class T>
    size_t SafeWrite(TWriter& w, const T& t);

    /// Same as above. Returned position will be relative
    /// to current stream position.
    template <class T>
    size_t SafeWrite(IOutputStream& out, const T& t);

    /// Aliases to safeWrite()
    template <class T>
    size_t Write(TWriter& w, const T& t) {
        return SafeWrite(w, t);
    }

    template <class T>
    size_t Write(IOutputStream& out, const T& t) {
        return SafeWrite(out, t);
    }

    /// Writes element without its version, thus making
    /// safeCast() inappropriate.
    template <class T>
    size_t UnsafeWrite(TWriter& w, const T& t);

    template <class T>
    size_t UnsafeWrite(IOutputStream& out, const T& t);

    /// Implementation

    template <class T>
    inline size_t SafeWrite(TWriter& w, const T& t) {
        mms::impl::layOut(w, t, true);
        return mms::impl::write(w, t, true);
    }

    template <class T>
    inline size_t SafeWrite(IOutputStream& out, const T& t) {
        TWriter w(out);
        mms::impl::layOut(w, t, true);
        return mms::impl::write(w, t, true);
    }

    template <class T>
    inline size_t UnsafeWrite(TWriter& w, const T& t) {
        mms::impl::layOut(w, t, false);
        return mms::impl::write(w, t, false);
    }

    template <class T>
    inline size_t UnsafeWrite(IOutputStream& out, const T& t) {
        TWriter w(out);
        mms::impl::layOut(w, t, false);
        return mms::impl::write(w, t, false);
    }

} //namespace NMms
