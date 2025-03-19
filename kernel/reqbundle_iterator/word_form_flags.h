#pragma once

#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/fwd.h>
#include <util/generic/vector.h>
#include <util/memory/pool.h>

class TResizableWordFormFlags {
public:
    TResizableWordFormFlags() = default;

    TResizableWordFormFlags(TMemoryPool& pool)
        : FormClasses_(&pool)
    {
    }

    void SetFormsCount(size_t formsCount) {
        FormClasses_.resize(formsCount, NUM_FORM_CLASSES);
    }

    size_t GetFormsCount() const {
        return FormClasses_.size();
    }

    EFormClass GetFormClass(size_t formIndex) const {
        Y_ASSERT(formIndex < GetFormsCount());
        return static_cast<EFormClass>(FormClasses_[formIndex]);
    }

    void SetFormClass(size_t formIndex, EFormClass formClass) {
        Y_ASSERT(formIndex < GetFormsCount());
        FormClasses_[formIndex] = static_cast<ui8>(formClass);
    }

    void SetAdditionalTokensCount(size_t /*formIndex*/, ui8 /*count*/) {
        // compatibility with TWordFormFlags for TGenericFormSelector
    }

    void SetIsBestLemma(bool /*isBestLemma*/) {
        // compatibility with TWordFormFlags for TGenericFormSelector
    }

private:
    TVector<ui8, TPoolAllocator> FormClasses_;
};
