#pragma once

#include <contrib/libs/mms/copy.h>

namespace NMms {
    /**
     * Converts a TMmapped record into its TStandalone counterpart.
     * For classes containing MMS_DECLARE_FIELDS or traverseFields(),
     * it copies every field recursively. For all others, it calls
     * TStandalone::operator=(const TMmapped&), hoping that it exists.
     */

    template <class TSA>
    inline void Copy(const typename mms::impl::MmappedType<TSA>::type& from, TSA& to) {
        mms::copy(from, to);
    }
}
