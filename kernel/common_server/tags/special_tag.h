#pragma once
#include "object.h"
#include "abstract.h"

namespace NCS {

    template <class TTagClass, class TDBTagInfo>
    class TDBTagSpecialImpl {
    private:
        TDBTagInfo TagData;
        mutable TMaybe<TDBTagDescription> TagDescription;
    public:
        const TString& GetTagId() const {
            return TagData.GetTagId();
        }

        TString& MutableTagId() {
            return TagData.MutableTagId();
        }

        const TString& GetObjectId() const {
            return TagData.GetObjectId();
        }

        const TDBTagInfo& GetDBTag() const {
            return TagData;
        }

        TDBTagInfo& MutableDBTag() {
            return TagData;
        }

        TMaybe<TDBTagDescription> GetDBTagDescriptionMaybe() const {
            return TagDescription;
        }

        TDBTagSpecialImpl() = default;

        template <class T>
        TDBTagSpecialImpl(const TDBTagSpecialImpl<T, TDBTagInfo>& tsSpecial)
            : TagData(tsSpecial.GetDBTag())
            , TagDescription(tsSpecial.GetDBTagDescriptionMaybe()) {
        }

        TDBTagSpecialImpl(const TDBTagInfo& tagData)
            : TagData(tagData) {
        }

        bool operator!() const {
            return !TagData.template GetPtrAs<TTagClass>();
        }

        const TDBTagDescription& GetDBTagDescription() const {
            if (!TagDescription) {
                TagDescription = ITagDescriptions::Instance().GetTagDescription(TagData->GetName());
            }
            return *TagDescription;
        }

        const typename TTagClass::TTagDescription& GetTagDescription() const {
            auto td = GetDBTagDescription();
            auto result = td.template GetVerifiedPtrAs<typename TTagClass::TTagDescription>();
            return *result;
        }

        TAtomicSharedPtr<typename TTagClass::TTagDescription> GetTagDescriptionPtr(const bool verify = false) const {
            auto td = GetDBTagDescription();
            auto result = td.template GetPtrAs<typename TTagClass::TTagDescription>();
            CHECK_WITH_LOG(!verify || !!result);
            return result;
        }

        TDBTagInfo GetDeepCopy() const {
            return TagData.GetDeepCopy();
        }

        const TTagClass* operator->() const {
            CHECK_WITH_LOG(TagData.template Is<TTagClass>());
            return TagData.template GetAs<TTagClass>();
        }

        TTagClass* operator->() {
            CHECK_WITH_LOG(TagData.template Is<TTagClass>());
            return TagData.template MutableAs<TTagClass>();
        }

        const TTagClass& operator*() const {
            CHECK_WITH_LOG(TagData.template Is<TTagClass>());
            return *TagData.template GetAs<TTagClass>();
        }
    };

}

template <class TTagClass>
class TDBTagSpecial: public NCS::TDBTagSpecialImpl<TTagClass, TDBTag> {
private:
    using TBase = NCS::TDBTagSpecialImpl<TTagClass, TDBTag>;
public:
    using TBase::TBase;
};
