#pragma once

#include <kernel/indexann_data/data.h>
#include <kernel/factor_storage/factor_storage.h>
#include <kernel/u_tracker/u_tracker.h>

#include <util/generic/cast.h>

// Will treat ann data as simple ui8 field, and cast it to float via Ui82Float!!!
template<NIndexAnn::EDataType FieldId, ui8 MaxCalcLevel = TUTrackerWrapper::MaxPossibleCalcLevel>
class TAnnUTrackerWrapper final : public TUTrackerWrapperWithLevel<MaxCalcLevel> {
private:
    typedef typename NIndexAnn::TStreamId2Type<FieldId>::TType TIndexAnnDataClass;
public:
    TAnnUTrackerWrapper()
    {
        static_assert(sizeof(TIndexAnnDataClass) == 1, "TIndexAnnDataClass is expected to have size 1.");
    }

    float GetWeight(ui32 data) const override
    {
        return Ui82Float(ui8(data));
    }

    size_t GetFieldId() const override
    {
        return FieldId;
    }

    TString GetFieldName() const override
    {
        return NIndexAnn::TDataTypeConcern::Instance().GetName(FieldId);
    }
};

// Will treat ann data as simple float field, and cast it to float via Ui82Float!!!
template<NIndexAnn::EDataType FieldId, int NormFactor, ui8 MaxCalcLevel = TUTrackerWrapper::MaxPossibleCalcLevel>
class TAnnUTrackerWrapperFloat final : public TUTrackerWrapperWithLevel<MaxCalcLevel> {
private:
    typedef typename NIndexAnn::TStreamId2Type<FieldId>::TType TIndexAnnDataClass;
public:
    TAnnUTrackerWrapperFloat()
    {
        static_assert(sizeof(TIndexAnnDataClass) == sizeof(float), "TIndexAnnDataClass is expected to have size of float.");
    }

    float GetWeight(ui32 data) const override
    {
        auto iData = BitCast<float>(data);
        return ClampVal<float>(iData / (iData + NormFactor), 0.0, 1.0);
    }

    size_t GetFieldId() const override
    {
        return FieldId;
    }

    TString GetFieldName() const override
    {
        return NIndexAnn::TDataTypeConcern::Instance().GetName(FieldId);
    }
};

// Will treat ann data as 1.0 field.
template<NIndexAnn::EDataType FieldId, ui8 MaxCalcLevel = TUTrackerWrapper::MaxPossibleCalcLevel>
class TAnnUTrackerWrapperEmpty final : public TUTrackerWrapperWithLevel<MaxCalcLevel> {
private:
    typedef typename NIndexAnn::TStreamId2Type<FieldId>::TType TIndexAnnDataClass;
public:
    TAnnUTrackerWrapperEmpty() = default;

    float GetWeight(ui32 /*data*/) const override
    {
        return 1.0;
    }

    size_t GetFieldId() const override
    {
        return FieldId;
    }

    TString GetFieldName() const override
    {
        return NIndexAnn::TDataTypeConcern::Instance().GetName(FieldId);
    }
};
