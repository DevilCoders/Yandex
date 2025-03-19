#pragma once

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <util/generic/string.h>
#include <util/generic/ptr.h>


// A simple text config in form of single gazetteer article.
// Allows describing configs of arbitrary level and complexity.
// A description of config object (TCfgProto) is done using standard ProtoBuf syntax.

// See arcadia_tests_data/wizard/img_translate/imgtranslate2.cfg or unittest for example.

namespace NProtoConf {

    bool LoadFromFile(const TString& fileName, const NProtoBuf::Descriptor* descr, NProtoBuf::Message& res);

    template <typename TCfgProto>
    inline bool LoadFromFile(const TString& fileName, TCfgProto& res) {
        return LoadFromFile(fileName, TCfgProto::descriptor(), res);
    }

    template <typename TCfgProto>
    inline TAutoPtr<TCfgProto> LoadFromFile(const TString& fileName) {
        TAutoPtr<TCfgProto> ret(new TCfgProto);
        return LoadFromFile(fileName, *ret) ? ret : nullptr;
    }

}   // namespace NProtoConf
