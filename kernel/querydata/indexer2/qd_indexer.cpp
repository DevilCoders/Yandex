#include "qd_indexer.h"
#include "qd_parser_utils.h"
#include "qd_factors_parser.h"

#include <kernel/querydata/common/qd_key_token.h>
#include <kernel/querydata/idl/querydata_structs_client.pb.h>

#include <util/generic/yexception.h>
#include <util/string/builder.h>

namespace NQueryData {

    void TRawParsedRecord::Clear() {
        Type = TRecordType();
        Key.clear();
        Value.clear();
    }

    static void DoParseRecord(TRawParsedRecord& rec, TStringBuf key, TStringBuf dir, TStringBuf value, const TIndexerSettings& opts) {
        rec.Clear();

        if (opts.HasDirectives) {
            rec.Type = ParseRecordType(dir);
        } else {
            rec.Type = TRecordType::M_QUERY;
        }

        rec.Key = key;
        rec.Value = value;

        TSubkeysCounts cnts = GetAllSubkeysCounts(key);

        if (rec.Type.IsQuery() && !cnts.Nonempty) {
            Y_ENSURE(opts.LegacyCommonAllowed, "legacy commons (empty keys) are not allowed");
            rec.Key.clear();
            rec.Type = TRecordType::M_COMMON;
        }

        if (rec.Type.IsCommon()) {
            Y_ENSURE(opts.Indexing.CommonAllowed, "commons are not allowed");
        } else {
            Y_ENSURE(!cnts.Empty, "empty subkeys are not allowed");
            ui32 expCnt = opts.Indexing.Subkeys.size();
            Y_ENSURE(cnts.Nonempty == expCnt, "subkeys counts do not match: " << cnts.Nonempty << " != " << expCnt << "(expected)");
        }

        if (rec.Type.IsKeyRef()) {
            Y_ENSURE(opts.Indexing.KeyRefAllowed, "keyrefs are not allowed");
        }
    }

    void ParseLocalRecord(TRawParsedRecord& rec, TStringBuf line, const TIndexerSettings& opts) {
        TStringBuf dir = opts.HasDirectives ? line.NextTok('\t') : "";
        TStringBuf key = SkipN(line, opts.Indexing.Subkeys.size());
        DoParseRecord(rec, key, dir, line, opts);
    }

    void ParseMRRecord(TRawParsedRecord& rec, TStringBuf key, TStringBuf subkey, TStringBuf value, const TIndexerSettings& opts) {
        DoParseRecord(rec, key, opts.HasDirectives ? subkey : "", value, opts);
    }

}
