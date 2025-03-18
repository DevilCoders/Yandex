#pragma once

#include <util/generic/buffer.h>
#include <util/generic/utility.h>
#include <util/system/align.h>
#include <util/system/yassert.h>

#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/key/key_output_buffer.h>

#include <cstddef>

namespace NOffroad {
    template <size_t blockSize>
    class TTrieOutputBuffer {
        using TKeyBuffer = TKeyOutputBuffer<std::nullptr_t, TNullVectorizer, TINSubtractor, blockSize, DeltaKeySubtractor>;
        using TDataBuffer = TKeyOutputBuffer<std::nullptr_t, TNullVectorizer, TINSubtractor, blockSize, IdentityKeySubtractor>;

    public:
        using TKey = TString;
        using TKeyRef = TStringBuf;
        using TData = TStringBuf;

        enum {
            TupleSize = TKeyBuffer::TupleSize + TDataBuffer::TupleSize,
            BlockSize = blockSize,
        };

        TTrieOutputBuffer() {
        }

        void Reset() {
            KeyBuffer_.Reset();
            DataBuffer_.Reset();
        }

        void Write(const TKeyRef& key, const TData& data) {
            Y_ASSERT(!IsDone());

            KeyBuffer_.Write(key, nullptr);
            DataBuffer_.Write(data, nullptr);
        }

        void Finish() {
            KeyBuffer_.Finish();
            DataBuffer_.Finish();
        }

        bool IsDone() const {
            return KeyBuffer_.IsDone();
        }

        template <class Writer>
        void Flush(Writer* writer) {
            static_assert(static_cast<int>(Writer::BlockSize) == static_cast<int>(BlockSize), "Expecting writer with compatible block size.");

            KeyBuffer_.Flush(writer);
            DataBuffer_.Flush(AsShiftedEncoder<TKeyBuffer::TupleSize>(writer));
        }

        size_t BlockPosition() const {
            return KeyBuffer_.BlockPosition();
        }

        const TString& LastKey() const {
            return KeyBuffer_.LastKey();
        }

        const TString& LastData() const {
            return DataBuffer_.LastKey();
        }

    private:
        TKeyBuffer KeyBuffer_;
        TDataBuffer DataBuffer_;
    };

}
