#include "slices_info.h"

#include "meta_info.h"

#include <util/generic/singleton.h>

namespace NFactorSlices {
    TStringBuf TSlicesInfo::GetName(EFactorSlice slice) const
    {
        return GetStaticSliceInfo(slice).Name;
    }
    TStringBuf TSlicesInfo::GetCppName(EFactorSlice slice) const
    {
        return GetStaticSliceInfo(slice).CppName;
    }

    const IFactorsInfo* TSlicesInfo::GetFactorsInfo(EFactorSlice slice) const
    {
        if (slice == EFactorSlice::COUNT) {
            return nullptr;
        }
        return TGlobalSlicesMetaInfo::Instance().GetFactorsInfo(slice);
    }

    bool TSlicesInfo::IsHierarchical(EFactorSlice slice) const
    {
        return GetStaticSliceInfo(slice).Hierarchical;
    }
    bool TSlicesInfo::IsLeaf(EFactorSlice slice) const
    {
        return !GetStaticSliceInfo(slice).Hierarchical;
    }
    bool TSlicesInfo::HasOneChild(EFactorSlice slice) const {
        const auto& info = GetStaticSliceInfo(slice);
        return info.Hierarchical &&
            GetStaticSliceInfo(info.FirstChild).NextSibling == EFactorSlice::COUNT;
    }

    EFactorSlice TSlicesInfo::GetParent(EFactorSlice slice) const
    {
        return GetStaticSliceInfo(slice).Parent;
    }
    EFactorSlice TSlicesInfo::GetNext(EFactorSlice slice) const
    {
        return GetStaticSliceInfo(slice).Next;
    }
    EFactorSlice TSlicesInfo::GetNextSibling(EFactorSlice slice) const
    {
        return GetStaticSliceInfo(slice).NextSibling;
    }
    EFactorSlice TSlicesInfo::GetFirstChild(EFactorSlice slice) const
    {
        return GetStaticSliceInfo(slice).FirstChild;
    }
    EFactorSlice TSlicesInfo::GetFirstLeaf(EFactorSlice slice) const
    {
        EFactorSlice leaf = slice;

        for (const TSliceStaticInfo* si = &GetStaticSliceInfo(leaf); si->Hierarchical; si = &GetStaticSliceInfo(leaf)) {
            leaf = si->FirstChild;
        }

        return leaf;
    }
    EFactorSlice TSlicesInfo::GetNextLeaf(EFactorSlice slice) const
    {
        while (slice != EFactorSlice::COUNT) {
            const TSliceStaticInfo& si = GetStaticSliceInfo(slice);
            if (si.NextSibling != EFactorSlice::COUNT) {
                return GetFirstLeaf(si.NextSibling);
            }
            slice = si.Parent;
        }

        return EFactorSlice::COUNT;
    }

    bool TSlicesInfo::IsChild(EFactorSlice parent, EFactorSlice slice) const
    {
        while (GetStaticSliceInfo(slice).Parent != EFactorSlice::COUNT) {
            slice = GetStaticSliceInfo(slice).Parent;
            if (slice == parent) {
                return true;
            }
        }
        return false;
    }

    EFactorSlice TSlicesInfo::GetSliceFor(EFactorUniverse universe, ESliceRole role) const
    {
        return NFactorSlices::GetStaticUniverseInfo(universe).SliceByRole[static_cast<size_t>(role)];
    }

    const TSlicesInfo* GetSlicesInfo() {
        return Singleton<TSlicesInfo>();
    }
}
