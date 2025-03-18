#pragma once

#include <util/generic/strbuf.h>

namespace NHtml5 {
    class TNoindexType {
    public:
        inline TNoindexType()
            : IsNoindex_(false)
            , IsClose_(false)
        {
        }

        inline TNoindexType(bool isNoindex, bool isClose)
            : IsNoindex_(isNoindex)
            , IsClose_(isClose)
        {
        }

        inline explicit operator bool() const {
            return IsNoindex_;
        }

        inline bool IsClose() const {
            return IsClose_;
        }

        inline bool IsNoindex() const {
            return IsNoindex_;
        }

    private:
        const bool IsNoindex_;
        const bool IsClose_;
    };

    TNoindexType DetectNoindex(const TStringBuf& text);
    TNoindexType DetectNoindex(const char* text, size_t len);

}
