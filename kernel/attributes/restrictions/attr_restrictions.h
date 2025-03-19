#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/system/yassert.h>

#include <kernel/qtree/request/nodebase.h>
#include <kernel/attributes/restrictions/proto/single_attr_restriction.pb.h>
#include <kernel/attributes/restrictions/proto/attr_restrictions.pb.h>

namespace NAttributes {

class TAttrRestrictionIteratorProto;
class TAttrRestrictionIteratorsProto;

/*
    See attr_restriction_decomposer.h for details
*/

template<typename T>
struct TAttrRestrictionsTraits {
};

template<>
struct TAttrRestrictionsTraits<TSingleAttrRestriction> {
    using Underlying = TAttrRestrictionsProto;
};

template<>
struct TAttrRestrictionsTraits<TAttrRestrictionIteratorProto> {
    using Underlying = TAttrRestrictionIteratorsProto;
};

template<typename T = TSingleAttrRestriction>
class TAttrRestrictions {
public:
    using Underlying = typename TAttrRestrictionsTraits<T>::Underlying;

    TAttrRestrictions() = default;

    explicit TAttrRestrictions(Underlying base)
        : Base_{std::move(base)}
    {}

    bool Empty() const {
        return Base_.GetTree().empty();
    }

    void Clear() {
        Base_.ClearTree();
    }

    const T& operator[](size_t i) const {
        return Base_.GetTree()[i];
    }

    T& operator[](size_t i) {
        return (*Base_.MutableTree())[i];
    }

    const Underlying& GetBase() const {
        return Base_;
    }

    const T& Back() const {
        Y_ASSERT(!Empty());
        return *Base_.GetTree().rbegin();
    }

    T& Back() {
        Y_ASSERT(!Empty());
        return *(Base_.MutableTree()->rbegin());
    }

    const T& Front() const {
        Y_ASSERT(!Empty());
        return *(Base_.GetTree().begin());
    }

    T& Front() {
        Y_ASSERT(!Empty());
        return *(Base_.MutableTree()->begin());
    }

    size_t Size() const {
        return Base_.TreeSize();
    }

    void AddNode(const T& node) {
        *(Base_.AddTree()) = node;
    }


    void AddNode(T&& node) {
        *(Base_.AddTree()) = node;
    }

    void PopBack() {
        Y_ASSERT(!Empty());
        Base_.MutableTree()->RemoveLast();
    }

    void Reserve(size_t size) {
        Base_.MutableTree()->Reserve(size);
    }

    TString SerializeToString() const {
        TString result;
        Y_PROTOBUF_SUPPRESS_NODISCARD Base_.SerializeToString(&result);
        return result;
    }

    bool ParseFromString(const TString &str) {
        return Base_.ParseFromString(str);
    }

private:
    Underlying Base_;
};


} // namespace NAttributes
