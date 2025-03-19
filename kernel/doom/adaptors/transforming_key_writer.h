#pragma once

#include <utility>

// TODO : merge with key transforming writer, update ns
namespace NOffroad {


template<class KeyTransformation, class Base>
class TTransformingKeyWriter : public Base {
public:
    using TKey = typename Base::TKey;
    using TKeyRef = typename Base::TKeyRef;
    using TKeyData = typename Base::TKeyData;

    using Base::Base;

    TTransformingKeyWriter() = default;

    template <class... Args>
    TTransformingKeyWriter(const KeyTransformation& keyTransformation, Args&&... args)
        : Base(std::forward<Args>(args)...)
        , KeyTransformation_(keyTransformation)
    {
    }

    void WriteKey(const TKeyRef& key, const TKeyData& data) {
        Base::WriteKey(KeyTransformation_(key), data);
    }

private:
    KeyTransformation KeyTransformation_;
};

struct TKeyFullPrefixGetter {
    TStringBuf operator()(const TStringBuf& key) const {
        return key;
    }
};

} // namespace NOffroad
