#include "article.h"
#include "blob.h"

#include <library/cpp/json/json_writer.h>
#include <library/cpp/protobuf/json/proto2json.h>

#include <util/charset/wide.h>
#include <util/generic/singleton.h>
#include <util/stream/str.h>

#include <google/protobuf/message.h>

namespace NRemorphAPI {

namespace NImpl {

namespace {

struct TJsonWriterConfig: NJson::TJsonWriterConfig {
    TJsonWriterConfig()
        : NJson::TJsonWriterConfig()
    {
        FormatOutput = false;
        SortKeys = false;
        ValidateUtf8 = false;
        DontEscapeStrings = false;
    }
};

}

TArticle::TArticle(const IBase* parent, const NGzt::TArticlePtr& a)
    : Lock(parent)
    , Article(a)
{
    Title = WideToUTF8(a.GetTitle());
}

const char* TArticle::GetType() const {
    return Article->GetTypeName().data();
}

const char* TArticle::GetName() const {
    return Title.data();
}

IBlob* TArticle::GetBlob() const {
    return new TBlob(Article.GetBinary());
}

IBlob* TArticle::GetJsonBlob() const {
    const NProtoBuf::Message* message = Article.Get();
    Y_ASSERT(message);
    TString jsonData;
    TStringOutput jsonStream(jsonData);
    NJson::TJsonWriter jsonWriter(&jsonStream, Default<TJsonWriterConfig>());
    NProtobufJson::Proto2Json(*message, jsonWriter);
    return new TBlob(jsonData);
}

} // NImpl

} // NRemorphAPI
