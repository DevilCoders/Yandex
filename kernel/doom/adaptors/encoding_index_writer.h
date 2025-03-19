#pragma once

#include <utility>                      /* For std::forward. */
#include <type_traits>                  /* For std::is_same. */

#include <kernel/search_types/search_types.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>

#include <kernel/doom/key/decoded_key.h>
#include <kernel/doom/key/old_key_encoder.h>

namespace NDoom {


template<class Base, class Encoder = TOldKeyEncoder>
class TEncodingIndexWriter: public Base {
    static_assert(std::is_same<typename Base::TKeyRef, TStringBuf>::value, "Base class is expected to use TStringBuf as key.");
public:
    using TKey = TDecodedKey;
    using TKeyRef = TDecodedKey;
    using THit = typename Base::THit;
    using TEncoder = Encoder;

    using Base::Base;
    using Base::WriteHit;

    void WriteKey(const TKeyRef& key) {
        if (!Encoder_.Encode(key, &Key_)) {
            /* Ok, so we've just run into an invalid key, and corresponding
             * hits have already been written out. The only option left is to
             * fix the key or use some other key that fits in. Here we don't
             * care fixing it, just incrementing the last one. */
            if (!IncrementKey(LastKey_))
                ythrow yexception() << "Can't fix invalid key '" << Key_ << "' while writing index.";

            LastKey_.swap(Key_);
        }

        Base::WriteKey(Key_);

        LastKey_.swap(Key_);
    }

private:
    static bool IncrementKey(TString& key) {
        if (key.size() < MAXKEY_LEN) {
            key += ' '; /* Space, 0x20, first printable char. */
            return true;
        }

        size_t index = key.size();
        do {
            index--;
            if (static_cast<unsigned char>(key[index]) < 255) {
                ++*(key.begin() + index);
                key.resize(index + 1);
                return true;
            }
        } while (index > 0);

        /* There's nothing we can do now. */
        return false;
    }

private:
    TEncoder Encoder_;
    TString LastKey_;
    TString Key_;
};


} // namespace NDoom
