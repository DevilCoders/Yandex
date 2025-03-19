#pragma once

#include "gazetteer.h"
#include "protoparser/builtin.h"

#include <kernel/gazetteer/proto/base.pb.h>

#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#include <library/cpp/langs/langs.h>
#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/generic/set.h>

class TGztOptions;

namespace NGzt {

// from syntax.proto
class TGztFileDescriptorProto;

class TGztGenerator {
public:
    // When @parentGzt is supplied, required proto's definitions (import "*.proto";)
    // will be taken from @parentGzt's descriptor pool instead of searching them on disk.
    TGztGenerator(const TGazetteer* parentGzt = nullptr, const TString& encoding = "utf8");

    ~TGztGenerator();

    void AddImport(const TString& fileName);
    void AddOptions(const TGztOptions& options);

    // If you have a ready protobuf-generated class TProto it is possible to use
    // its builtin descriptor definition from binary (generated_pool), without doing explicit import.
    template <typename TProto>
    void UseBuiltinDescriptor() {
        BuiltinProto.AddFileSafe<TProto>();
        AddImport(TProto::descriptor()->file()->name());
    }

    void UseAllBuiltinDescriptors() {
        BuiltinProto.Maximize();
    }

    // direct protobuf article, don't add necessary imports into Collection
    void AddArticle(const NProtoBuf::Message& art, const TString& encodedTitle = TString());

    // Compile all collected so far articles into new ready gazetteer.
    TAutoPtr<TGazetteer> MakeGazetteer() const;

    TAutoPtr<TGazetteerBuilder> MakeBuilder() const;

    // key generation helpers for common usage cases

    static TSearchKey* AddKey(NProtoBuf::RepeatedPtrField<TSearchKey>& keys, const TString& encodedText,
                              TMorphMatchRule::EType morph = TMorphMatchRule::ALL_FORMS, ELanguage lang = LANG_UNK);

    template <typename TArticleSubType>
    static TSearchKey* AddKey(TArticleSubType& art, const TString& encodedKey) {
        // default: key is lemma
        return AddKey(*art.mutable_key(), encodedKey, TMorphMatchRule::ALL_FORMS);
    }

    template <typename TArticleSubType>
    static TSearchKey* AddExactFormKey(TArticleSubType& art, const TString& encodedKey) {
        return AddKey(*art.mutable_key(), encodedKey, TMorphMatchRule::EXACT_FORM);
    }

    template <typename TArticleSubType>
    static TSearchKey* AddAllLemmaFormsKey(TArticleSubType& art, const TString& encodedKey, ELanguage lang) {
        // lemmer lang must be specified for ALL_LEMMA_FORMS, otherwise russian is used.
        return AddKey(*art.mutable_key(), encodedKey, TMorphMatchRule::ALL_LEMMA_FORMS, lang);
    }

    TString DebugString();

private:
    const TGazetteer* ParentGzt;
    NBuiltin::TFileCollection BuiltinProto;

    THolder<TGztFileDescriptorProto> GeneratedFile;
};



}   // namespace NGzt
