#include "lemmer_cache.h"
#include <kernel/qtree/richrequest/protos/lemmer_cache_key.pb.h>
#include <library/cpp/token/serialization/protos/char_span.pb.h>
#include <library/cpp/token/serialization/serialize_char_span.h>
#include <ysite/yandex/reqdata/reqdata.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/protobuf/json/json2proto.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/langmask/serialization/langmask.h>
#include <util/digest/numeric.h>

bool NLemmerCache::TKey::operator==(const TKey& other) const {
    return Word == other.Word && Span == other.Span && Type == other.Type &&
        LangMask == other.LangMask &&
        FilterForms == other.FilterForms;
}

size_t NLemmerCache::TKey::Hash() const {
    size_t result = THash<TUtf16String>()(Word);
    result = CombineHashes(result, THash<TCharSpan>()(Span));
    result = CombineHashes(result, size_t(Type));
    result = CombineHashes(result, LangMask.GetHash());
    result = CombineHashes(result, THash<bool>()(FilterForms));
    return result;
}

TString NLemmerCache::TKey::ToJson() const {
    NLemmerCacheProto::TKey message;
    message.SetWord(WideToUTF8(Word));
    SerializeCharSpan(Span, *message.MutableSpan());
    message.SetType(Type);
    Serialize(*message.MutableLangMask(), LangMask, true);
    message.SetFilterForms(FilterForms);

    NJson::TJsonValue result;
    NProtobufJson::Proto2Json(message, result);
    TStringStream output;
    NJson::WriteJson(&output, &result);
    return output.Str();
}

void NLemmerCache::TKey::FromJson(const TString& json) {
    NJson::TJsonValue value;
    TStringStream input;
    input.Str() = json;
    NJson::ReadJsonTree(&input, &value);
    NLemmerCacheProto::TKey message;
    NProtobufJson::Json2Proto(value, message);

    Word = UTF8ToWide(message.GetWord());
    DeserializeCharSpan(Span, message.GetSpan());
    Type = TFormType(message.GetType());
    LangMask = Deserialize(message.GetLangMask());
    FilterForms = message.GetFilterForms();
}

//-------------------------------------------------------------------------

void TLemmerCache::Clear() {
    TWriteGuard guard(Mutex);
    Body.clear();
}

void TLemmerCache::SetExternals(const TRequesterData& externals) {
    TWriteGuard guard(Mutex);
    StopWords = &externals.WordFilter;
    Decimator = &externals.Decimator;
 }

bool TLemmerCache::HasCorrectExternals(const TLanguageContext& context) const {
    TReadGuard guard(Mutex);
    return context.GetLangOrder() == nullptr &&
        &context.GetDecimator() == Decimator &&
        &context.GetStopWords() == StopWords;
}

bool TLemmerCache::Find(const TKey& key, TWordInstance& result) const {
    TReadGuard guard(Mutex);
    TBody::const_iterator pos = Body.find(key);
    if (pos == Body.end()) {
        return false;
    }
    result = pos->second;
    return true;
}

void TLemmerCache::Set(const TKey& key, TWordInstance instance) {
    TWriteGuard guard(Mutex);
    Body[key] = instance;
}
