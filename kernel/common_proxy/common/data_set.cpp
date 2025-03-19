#include "data_set.h"
#include <util/generic/set.h>
#include <util/string/vector.h>

namespace NCommonProxy {

    TString TMetaData::TValueMeta::ToString() const {
        if (Type == dtOBJECT) {
            CHECK_WITH_LOG(ObjectChecker);
            return ObjectChecker->Type();
        }
        return ::ToString(Type);
    }

    TMetaData::TMetaData()
    {}

    bool TMetaData::IsSubsetOf(const TMetaData& other) const {
        for (const auto& field : Fields) {
            auto otherField = other.Fields.find(field.first);
            if (otherField == other.Fields.end() || otherField->second.Type != field.second.Type)
                return false;
        }
        return true;
    }

    bool TMetaData::TypeIs(const TString& name, TDataType type) const {
        auto field = Fields.find(name);
        return field != Fields.end() && field->second.Type == type;
    }

    void TMetaData::Register(const TString& name, const TValueMeta& meta) {
        CHECK_WITH_LOG(Fields.insert(std::make_pair(name, meta)).second);
    }

    TString TMetaData::ToString() const {
        TMap<TString, TValueMeta> sorted(Fields.begin(), Fields.end());
        TVector<TString> result;
        result.reserve(sorted.size());
        for (const auto& i : sorted)
            result.push_back(i.second.ToString() + " " + i.first);
        return "(" + JoinStrings(result, ",") + ")";
    }

    const TMetaData TMetaData::Empty;

    TDataSet::TDataSet(const TMetaData& metaData)
        : MetaData(metaData)
    {}

    bool TDataSet::AllFieldsSet() const {
        return SetFields.size() == MetaData.GetFields().size();
    }

    namespace {

        template<class T>
        inline typename TTypeTraits<T>::TFuncParam Get(const THashMap<TString, T>& data, const TString& name, const TMetaData& metadata, TMetaData::TDataType type) {
            if (!metadata.TypeIs(name, type))
                ythrow yexception() << "Cannot get " << name << ": it is not " << type;
            auto i = data.find(name);
            if (i == data.end())
                ythrow yexception() << "Cannot get " << name << ": it is not set";
            return i->second;
        }

        template<class T>
        inline void Set(THashMap<TString, T>& data, const TString& name, const TMetaData& metadata, TMetaData::TDataType type, typename TTypeTraits<T>::TFuncParam value) {
            if (!metadata.TypeIs(name, type))
                ythrow yexception() << "Cannot set " << name << ": it is not " << type;
            data[name] = value;
        }

    }

    template<>
    TTypeTraits<i64>::TFuncParam TDataSet::Get<i64>(const TString& name) const {
        return NCommonProxy::Get(Ints, name, MetaData, TMetaData::dtINT);
    }

    template<>
    void TDataSet::SetImpl<i64>(const TString& name, TTypeTraits<i64>::TFuncParam value) {
        NCommonProxy::Set(Ints, name, MetaData, TMetaData::dtINT, value);
    }

    template<>
    TTypeTraits<bool>::TFuncParam TDataSet::Get<bool>(const TString& name) const {
        return NCommonProxy::Get(Bools, name, MetaData, TMetaData::dtBOOL);
    }

    template<>
    void TDataSet::SetImpl<bool>(const TString& name, TTypeTraits<bool>::TFuncParam value) {
        NCommonProxy::Set(Bools, name, MetaData, TMetaData::dtBOOL, value);
    }

    template<>
    TTypeTraits<TBlob>::TFuncParam TDataSet::Get<TBlob>(const TString& name) const {
        return NCommonProxy::Get(Blobs, name, MetaData, TMetaData::dtBLOB);
    }

    template<>
    void TDataSet::SetImpl<TBlob>(const TString& name, TTypeTraits<TBlob>::TFuncParam value) {
        NCommonProxy::Set(Blobs, name, MetaData, TMetaData::dtBLOB, value);
    }

    template<>
    TTypeTraits<double>::TFuncParam TDataSet::Get<double>(const TString& name) const {
        return NCommonProxy::Get(Doubles, name, MetaData, TMetaData::dtDOUBLE);
    }

    template<>
    void TDataSet::SetImpl<double>(const TString& name, TTypeTraits<double>::TFuncParam value) {
        NCommonProxy::Set(Doubles, name, MetaData, TMetaData::dtDOUBLE, value);
    }

    template<>
    TTypeTraits<TString>::TFuncParam TDataSet::Get<TString>(const TString& name) const {
        return NCommonProxy::Get(Strings, name, MetaData, TMetaData::dtSTRING);
    }

    template<>
    void TDataSet::SetImpl<TString>(const TString& name, TTypeTraits<TString>::TFuncParam value) {
        NCommonProxy::Set(Strings, name, MetaData, TMetaData::dtSTRING, value);
    }

    template<>
    TTypeTraits<TObject::TPtr>::TFuncParam TDataSet::Get<TObject::TPtr>(const TString& name) const {
        return NCommonProxy::Get(Objects, name, MetaData, TMetaData::dtOBJECT);
    }

    template<>
    void TDataSet::SetImpl<TObject::TPtr>(const TString& name, TTypeTraits<TObject::TPtr>::TFuncParam value) {
        NCommonProxy::Set(Objects, name, MetaData, TMetaData::dtOBJECT, value);
    }

    NCommonProxy::TObject::TPtr TDataSet::GetObjectPtr(const TString& name) const {
        if (!MetaData.TypeIs(name, TMetaData::dtOBJECT))
            ythrow yexception() << "Cannot get " << name << ": it is not " << TMetaData::dtOBJECT;
        auto i = Objects.find(name);
        if (i == Objects.end()) {
            return nullptr;
        }
        return i->second;
    }
}
