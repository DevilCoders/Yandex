#pragma once

#include <kernel/generated_factors_info/metadata/factors_metadata.pb.h>
#include <kernel/factors_codegen/factors_codegen.h>

struct TWebCodegenTraits
    : public TFactorsCodegenDefaultTraits
{
    static const bool UseFactorSlices = true;
    static const bool UseExtJsonInfo = true;
};

using TWebCodegen = TFactorsCodegen<NFactor::TCodeGenInput, TWebCodegenTraits>;
using TWebSlicesCodegen = TWebCodegen::TSlicesCodegen;

template<>
inline TString TWebCodegen::GetTagName(const NFactor::TCodeGenInput::TFactorDescriptor& descr, size_t t) const {
    const auto& tag = descr.GetTags(t);
    return NFactor::ETag_Name(tag);
}

template <>
inline TString TWebCodegen::GetComment(const NFactor::TCodeGenInput::TFactorDescriptor& descr) const {
    return descr.GetComment();
}

template <>
inline const Ngp::EnumDescriptor& TWebCodegen::GetTagDescriptor() const {
    return *NFactor::ETag_descriptor();
}

template <>
inline bool TWebCodegen::NeedOffsetGators(NFactor::TCodeGenInput& input) const {
    return input.OffsetSize() > 0;
}

template <>
inline void TWebCodegen::GenOffsetGators(NFactor::TCodeGenInput& input, TStringStream& protectedFactorStorage) {
    for (size_t i = 0; i != input.OffsetSize(); ++i) {
        GenOffsetGator(input.GetOffset(i).GetName(), input.GetOffset(i).GetIndex(), protectedFactorStorage);
    }
}

template <>
inline void TWebCodegen::CheckFactorRequiredFields(NFactor::TCodeGenInput& /*input*/) {
}

template<>
inline void TWebCodegen::CheckTagsRestrictions(NFactor::TCodeGenInput& input) {
    /* Check uniqueness for TG_L3_MODEL_VALUE tag */
    size_t foundL3ModelValueFactors = 0;
    for (const auto& factor : input.GetFactor()) {
        if (Find(factor.GetTags().begin(), factor.GetTags().end(), NFactor::TG_L3_MODEL_VALUE) != factor.GetTags().end()) {
            ++foundL3ModelValueFactors;
        }
    }
    if (foundL3ModelValueFactors > 1) {
        ythrow TGenError() << "At most one factor can have TG_L3_MODEL_VALUE tag";
    }
}

template<>
void TWebCodegen::CutUnwantedSlices(NFactor::TCodeGenInput& input, TCodegenParams& params) {
    if (!params.TargetSlice) return;

    auto target_ptr = input.MutableSlice()->begin();
    for (auto cur = input.MutableSlice()->begin(); cur != input.MutableSlice()->end(); ++cur) {
        if (cur->GetName() == params.TargetSlice) {
            input.MutableSlice()->erase(input.MutableSlice()->begin(), cur);
            target_ptr = input.MutableSlice()->begin() + 1;
            break;
        }
    }
    input.MutableSlice()->erase(target_ptr, input.MutableSlice()->end());
}

