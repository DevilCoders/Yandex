#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/buffer.h>

namespace NOffroad {
    namespace NPrivate {
        inline TStringBuf AsStringBuf(const TBuffer& buffer) {
            return TStringBuf(buffer.data(), buffer.size());
        }

        inline TStringBuf AsStringBuf(const TStringBuf& buffer) {
            return buffer;
        }

    }
}

#ifndef Y_NO_MD5_TESTS
#define UNIT_ASSERT_MD5_EQUAL(MEMORY, MD5_STRING) \
    UNIT_ASSERT_VALUES_EQUAL(MD5::Calc(::NOffroad::NPrivate::AsStringBuf(MEMORY)), MD5_STRING)
#else
#define UNIT_ASSERT_MD5_EQUAL(MEMORY, MD5_STRING) \
    Cerr << MD5_STRING << " -> " << MD5::Calc(::NOffroad::NPrivate::AsStringBuf(MEMORY)) << Endl;
#endif
