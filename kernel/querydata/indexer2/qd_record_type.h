#pragma once

#include <util/generic/strbuf.h>

namespace NQueryData {

    extern const TStringBuf QUERY_KEYWORD;
    extern const TStringBuf KEYREF_KEYWORD;
    extern const TStringBuf COMMON_KEYWORD;
    extern const TStringBuf TSTAMP_KEYWORD;

    extern const TStringBuf QUERY_DIRECTIVE;
    extern const TStringBuf KEYREF_DIRECTIVE;
    extern const TStringBuf COMMON_DIRECTIVE;
    extern const TStringBuf KEYREF_OLD_MR_DIRECTIVE;

    struct TRecordType {
        enum EMode {
            M_NONE = 0 /* "none" */,
            M_QUERY    /* "query" */,
            M_KEYREF   /* "keyref" */,
            M_COMMON   /* "common" */
        };

        EMode Mode = M_NONE;
        ui64 Timestamp = 0;

        TRecordType(EMode mode = M_NONE, ui64 tstamp = 0)
            : Mode(mode)
            , Timestamp(tstamp)
        {}

        bool IsQuery() const {
            return M_QUERY == Mode;
        }

        bool IsKeyRef() const {
            return M_KEYREF == Mode;
        }

        bool IsCommon() const {
            return M_COMMON == Mode;
        }

        TString ToString() const;
    };


    TRecordType ParseRecordType(TStringBuf type);

}
