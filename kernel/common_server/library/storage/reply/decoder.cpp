#include "decoder.h"
#include <library/cpp/json/json_reader.h>
#include <kernel/common_server/library/storage/records/db_value.h>
#include <kernel/common_server/library/storage/proto/packed_data.pb.h>
#include <library/cpp/blockcodecs/core/register.h>
#include <kernel/common_server/util/enum_cast.h>

namespace NCS {
    namespace NStorage {
        bool TBaseDecoder::GetJsonValue(const i32 idx, const TConstArrayRef<TStringBuf>& values, NJson::TJsonValue& jsonResult, const bool mayBeEmpty /*= true*/) const {
            try {
                if (idx >= 0) {
                    CHECK_WITH_LOG(values.size() > (size_t)idx);
                    if (!values[idx] && !mayBeEmpty) {
                        TFLEventLog::Log("field cannot by empty on decoding")("field_idx", idx)("values", JoinSeq(", ", values));
                        return false;
                    } else if (!values[idx]) {
                        jsonResult = NJson::JSON_NULL;
                        return true;
                    }
                    if (!NJson::ReadJsonFastTree(values[idx], &jsonResult)) {
                        TFLEventLog::Log("field must be json")("field_idx", idx)("values", JoinSeq(", ", values));
                        return false;
                    }
                    return true;
                } else {
                    return false;
                }
            } catch (...) {
                ERROR_LOG << CurrentExceptionMessage() << Endl;
                return false;
            }
        }

        bool TBaseDecoder::GetValueBytes(const i32 idx, const TConstArrayRef<TStringBuf>& values, TString& value) const noexcept {
            try {
                if (idx >= 0) {
                    CHECK_WITH_LOG(values.size() > (size_t)idx);
                    value = TString(values[idx].data(), values[idx].size());
                    return true;
                } else {
                    TFLEventLog::Error("incorrect index for bytes field")("idx", idx);
                    return false;
                }
            } catch (...) {
                TFLEventLog::Error("cannot read bytes field")("idx", idx)("reason", CurrentExceptionMessage());
                return false;
            }
        }

        bool TBaseDecoder::GetValueBytesPackedAuto(const i32 idx, const TConstArrayRef<TStringBuf>& values, TString& value) const noexcept {
            NCSProto::TPackedData protoPacked;
            if (!GetProtoValueBytes(idx, values, protoPacked)) {
                return false;
            }

            EDataCodec codec;
            if (!TEnumWorker<EDataCodec>::TryParseFromInt(protoPacked.GetCodec(), codec)) {
                TFLEventLog::Error("cannot determine codec")("codec", protoPacked.GetCodec());
                return false;
            }

            try {
                const NBlockCodecs::ICodec* c = NBlockCodecs::Codec(::ToString(codec));
                CHECK_WITH_LOG(c);
                value = c->Decode(protoPacked.GetData());
            } catch (...) {
                TFLEventLog::Error("cannot decode string")("codec", codec);
                return false;
            }
            return true;
        }

        bool TBaseDecoder::GetValueBytesPacked(const EDataCodec codec, const i32 idx, const TConstArrayRef<TStringBuf>& values, TString& value) const noexcept {
            TString valuePacked;
            if (!GetValueBytes(idx, values, valuePacked)) {
                return false;
            }
            try {
                const NBlockCodecs::ICodec* c = NBlockCodecs::Codec(::ToString(codec));
                CHECK_WITH_LOG(c);
                value = c->Decode(valuePacked);
            } catch (...) {
                TFLEventLog::Error("cannot decode string")("codec", codec);
                return false;
            }
            return true;
        }

        bool TBaseDecoder::GetRemap(const NJson::TJsonValue& jsonInfo, TVector<TString>& values, TVector<TStringBuf>& valuesBuf, TVector<TOrderedColumn>& remap) {
            const NJson::TJsonValue::TMapType* mPtr;
            if (!jsonInfo.GetMapPointer(&mPtr)) {
                return false;
            }
            TVector<TOrderedColumn> remapResult;
            values.reserve(mPtr->size());
            valuesBuf.reserve(mPtr->size());
            for (auto&& i : *mPtr) {
                values.emplace_back(i.second.IsDefined() ? i.second.GetStringRobust() : "");
                valuesBuf.emplace_back(values.back());
                const TMaybe<EColumnType> dbType = TDBValueOperator::FromJsonType(i.second.GetType());
                if (!dbType) {
                    TFLEventLog::Error("incorrect json value type")("type", i.second.GetType());
                    return false;
                }
                remapResult.emplace_back(TOrderedColumn(i.first, *dbType));
            }
            std::swap(remapResult, remap);
            return true;
        }

    }
}
