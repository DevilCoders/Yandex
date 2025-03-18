#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>

/*!
 * See README.md for documentation
 */
namespace NAuthClientParser {
    using TUid = ui64;

    class TOAuthToken {
    public:
        bool Parse(TStringBuf token);

        TUid Uid() const {
            Y_ENSURE(IsSucceed_);
            return Uid_;
        }

    private:
        bool ParseImpl(TStringBuf token);
        bool FromEmbeddedV2(TStringBuf token);
        bool FromEmbeddedV3(TStringBuf token);
        bool FromStateless(TStringBuf token);
        bool IsBase64Url(TStringBuf str);

        ui64 FromBytes(const char* bytes, size_t size = 8);

    private:
        TUid Uid_ = 0;
        bool IsSucceed_ = false;
    };
}
