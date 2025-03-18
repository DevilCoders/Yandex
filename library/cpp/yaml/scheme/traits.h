#pragma once

#include <util/stream/input.h>
#include <util/stream/file.h>
#include <util/generic/string.h>
#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/system/yassert.h>
#include <util/string/cast.h>

#include <contrib/libs/yaml/include/yaml.h>

struct TYamlTraits {
    struct TValueRef {
        yaml_node_s* N;
        yaml_document_s* D;

        inline TStringBuf Scalar() const {
            if (N && (N->type & YAML_SCALAR_NODE)) {
                return TStringBuf((const char*)N->data.scalar.value, N->data.scalar.length);
            }

            ythrow yexception() << "not a scalar";
        }
    };

    using TConstValueRef = TValueRef;
    using TStringType = TStringBuf;

    // common ops
    static inline bool IsNull(const TConstValueRef& v) noexcept {
        return !v.N;
    }

    // struct ops
    static inline TConstValueRef GetField(const TConstValueRef& v, TStringBuf name) noexcept {
        try {
            return DictElement(v, name);
        } catch (...) {
        }

        return {nullptr, v.D};
    }

    // array ops
    static bool IsArray(const TConstValueRef& v) noexcept {
        if (IsDict(v)) {
            return false;
        }

        return v.N && (v.N->type & YAML_SEQUENCE_NODE);
    }

    using TArrayIterator = yaml_node_item_t*;

    static inline TArrayIterator ArrayBegin(const TConstValueRef& v) noexcept {
        Y_VERIFY(v.N, "shit happen");

        return v.N->data.sequence.items.start;
    }

    static inline TArrayIterator ArrayEnd(const TConstValueRef& v) noexcept {
        Y_VERIFY(v.N, "shit happen");

        return v.N->data.sequence.items.top;
    }

    static inline TValueRef ArrayElement(const TConstValueRef& v, TArrayIterator n) {
        return {yaml_document_get_node(v.D, *n), v.D};
    }

    static inline size_t ArraySize(const TConstValueRef& v) {
        return ArrayEnd(v) - ArrayBegin(v);
    }

    // dict ops
    static bool IsDict(const TConstValueRef& v) noexcept {
        return v.N && (v.N->type & YAML_MAPPING_NODE);
    }

    using TDictIterator = yaml_node_pair_t*;

    static inline TDictIterator DictBegin(const TConstValueRef& v) noexcept {
        Y_VERIFY(v.N, "shit happen");

        return v.N->data.mapping.pairs.start;
    }

    static inline TDictIterator DictEnd(const TConstValueRef& v) noexcept {
        Y_VERIFY(v.N, "shit happen");

        return v.N->data.mapping.pairs.top;
    }

    static inline TStringBuf DictIteratorKey(const TConstValueRef& v, TDictIterator it) noexcept {
        auto n = yaml_document_get_node(v.D, it->key);

        Y_VERIFY(n, "shit happen");

        return TStringBuf((const char*)n->data.scalar.value, n->data.scalar.length);
    }

    static inline TConstValueRef DictIteratorValue(const TConstValueRef& v, TDictIterator it) noexcept {
        auto n = yaml_document_get_node(v.D, it->value);

        Y_VERIFY(n, "shit happen");

        return {n, v.D};
    }

    static inline TConstValueRef DictElement(const TConstValueRef& v, TStringBuf key) {
        for (auto it = DictBegin(v); it != DictEnd(v); ++it) {
            if (DictIteratorKey(v, it) == key) {
                return DictIteratorValue(v, it);
            }
        }

        ythrow yexception() << "no such key " << key;
    }

    static inline void DictSize(TConstValueRef) {
    }

    // generic get
    template <typename T>
    static inline void Get(TConstValueRef v, T def, T& t) {
        try {
            t = FromString<T>(v.Scalar());
        } catch (...) {
            t = def;
        }
    }

    template <typename T>
    static inline void Get(TConstValueRef v, T& t) {
        t = FromString<T>(v.Scalar());
    }

    template <typename T>
    static inline bool IsValidPrimitive(const T&, TConstValueRef v) {
        if (IsNull(v)) {
            return true;
        }

        try {
            FromString<T>(v.Scalar());

            return true;
        } catch (...) {
        }

        return false;
    }

    // validation ops
    static inline TVector<TString> GetKeys(const TConstValueRef& v) {
        TVector<TString> res;

        for (auto it = DictBegin(v); it != DictEnd(v); ++it) {
            res.emplace_back(ToString(DictIteratorKey(v, it)));
        }

        Sort(res.begin(), res.end());

        return res;
    }
};

struct TYamlDocument {
    yaml_document_s Doc;

    inline TYamlDocument(const TString& data) {
        memset(&Doc, 0, sizeof(Doc));
        yaml_parser_s parser;

        memset(&parser, 0, sizeof(parser));

        yaml_parser_initialize(&parser);
        yaml_parser_set_input_string(&parser, (const unsigned char*)data.data(), data.size());

        if (!yaml_parser_load(&parser, &Doc)) {
            ythrow yexception() << "can not parse yaml";
        }

        yaml_parser_delete(&parser);
    }

    inline TYamlDocument(IInputStream&& stream)
        : TYamlDocument(stream.ReadAll())
    {
    }

    inline ~TYamlDocument() {
        yaml_document_delete(&Doc);
    }

    inline TYamlTraits::TValueRef Root() {
        return {yaml_document_get_root_node(&Doc), &Doc};
    }
};
