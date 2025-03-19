#pragma once

#include "qd_limits.h"
#include "qd_record_type.h"

#include <kernel/querydata/common/qd_types.h>

#include <library/cpp/binsaver/bin_saver.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <util/stream/output.h>

namespace NQueryData {

    struct TLocalMetadata {
        ui64 DataTimestamp = 0;

        SAVELOAD(DataTimestamp);
    };


    struct TIndexingSettings {
        TKeyTypes Subkeys;
        bool KeyRefAllowed = false;
        bool CommonAllowed = false;

        SAVELOAD(Subkeys, KeyRefAllowed, CommonAllowed);
    };


    struct TIndexerSettings {
        TIndexingSettings Indexing;
        TLocalMetadata Local;

        bool LegacyCommonAllowed = false;
        bool HasDirectives = true;
        bool Json = false;
        bool Validate = false;

        SAVELOAD(Indexing, Local, LegacyCommonAllowed, HasDirectives, Json, Validate);
    };


    struct TRawParsedRecord {
        TRecordType Type;
        TString Key;
        TString Value;

        TRawParsedRecord() = default;

        TRawParsedRecord(const TRecordType& t, TStringBuf k, TStringBuf v)
            : Type(t)
            , Key(k)
            , Value(v)
        {}

    public:
        void Clear();
    };


    class IQDWriter : public TThrRefBase {
    public:
        using TRef = TIntrusivePtr<IQDWriter>;

        virtual void WriteRecord(const TStringBuf& key, const TStringBuf& val) = 0;
        virtual void Finalize() {}
    };


    class IQDIndexer : public TThrRefBase {
    public:
        using TRef = TIntrusivePtr<IQDIndexer>;

        virtual void ProcessRecord(const TRawParsedRecord&) = 0;
    };


    void ParseLocalRecord(TRawParsedRecord&, TStringBuf line, const TIndexerSettings&);


    void ParseMRRecord(TRawParsedRecord&, TStringBuf key, TStringBuf subkey, TStringBuf value, const TIndexerSettings&);
}
