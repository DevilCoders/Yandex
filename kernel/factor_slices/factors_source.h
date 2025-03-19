#pragma once

#include <util/generic/deque.h>
#include <util/generic/hash.h>
#include <util/generic/cast.h>
#include <util/generic/singleton.h>
#include <util/generic/yexception.h>

template <typename CodeGen, typename SliceGen = typename CodeGen::TSliceDescriptor>
class IFactorsSource {
public:
    using TCodeGenInput = CodeGen;
    using TSliceGenInput = SliceGen;

    virtual ~IFactorsSource() {}
    virtual ui32 UpdateCodegen(TSliceGenInput& sliceDescr, ui32 firstIndex) const = 0;

    template <typename TFactorsSource>
    TFactorsSource& To() {
        return *VerifyDynamicCast<TFactorsSource*>(this);
    }

protected:
    virtual void GenerateFactors(TSliceGenInput& input, ui32 firstIndex) const = 0;

};

template <typename CodeGen, typename SliceType, SliceType SliceValue>
class TFactorsSourceBase
    : public IFactorsSource<CodeGen>
{
public:
    using TCodeGenInput = typename IFactorsSource<CodeGen>::TCodeGenInput;
    using TSliceGenInput = typename IFactorsSource<CodeGen>::TSliceGenInput;
    using TFactorSlice = SliceType;
    static const TFactorSlice Slice = SliceValue;

    ui32 UpdateCodegen(TSliceGenInput& input, ui32 firstIndex) const override {
        ui32 savedNumFactors = input.FactorSize();
        this->GenerateFactors(input, firstIndex);
        ui32 maxIndex = firstIndex;
        for (ui32 i = savedNumFactors; i < input.FactorSize(); ++i) {
            Y_VERIFY(input.GetFactor(i).GetIndex() >= firstIndex,
                "unexpected index for generated factor");
            maxIndex = Max<ui32>(maxIndex, input.GetFactor(i).GetIndex());
        }
        return maxIndex + 1;
    }
};

template <typename SourceType>
void RegisterFactorsSource();

namespace NFactorsSourcePrivate {
    template <typename CodeGen, typename SliceType>
    struct TFactorsSrcRegistry {
    public:
        using TCodeGenInput = CodeGen;
        using TFactorSlice = SliceType;
        using ISrc = IFactorsSource<TCodeGenInput>;

        void Register(TFactorSlice slice, ISrc* src) {
            TString name = ToString(slice);
            Y_VERIFY(!Registry.has(slice), "factors source \"%s\" is registered more than once", name.data());
            Y_VERIFY(src, "null object for source \"%s\"", name.data());
            Registry[slice] = src;
        }

        bool Lookup(TFactorSlice slice, ISrc*& src) {
            auto iter = Registry.find(slice);
            if (iter == Registry.end())
                return false;
            src = iter->second;
            return true;
        }

        ISrc& Get(TFactorSlice slice) {
            TString name = ToString(slice);
            ISrc* src = nullptr;
            Y_ENSURE(Lookup(slice, src), "factors source " << name << " was not registered");
            return *src;
        }

        static TFactorsSrcRegistry<TCodeGenInput, TFactorSlice>& Instance() {
            return *Singleton<TFactorsSrcRegistry<TCodeGenInput, TFactorSlice>>();
        }

    private:
        THashMap<TFactorSlice, ISrc*> Registry;
    };

    template <typename SourceType>
    class TRegisterSourceHelper {
    public:
        TRegisterSourceHelper() {
            RegisterFactorsSource<SourceType>();
        }
    };
}

template <typename SourceType>
void RegisterFactorsSource() {
    using TCodeGenInput = typename SourceType::TCodeGenInput;
    using TFactorSlice = typename SourceType::TFactorSlice;
    using TRegistry = ::NFactorsSourcePrivate::TFactorsSrcRegistry<TCodeGenInput, TFactorSlice>;

    static const TFactorSlice slice = SourceType::Slice;
    static SourceType source{};

    TRegistry::Instance().Register(slice, &source);
}

template <typename CodeGen, typename SliceType>
IFactorsSource<CodeGen>& GetFactorsSource(SliceType slice) {
    using TRegistry = ::NFactorsSourcePrivate::TFactorsSrcRegistry<CodeGen, SliceType>;

    return TRegistry::Instance().Get(slice);
}

template <typename SourceType>
Y_FORCE_INLINE SourceType& GetFactorsSource() {
    return GetFactorsSource<typename SourceType::TCodeGenInput,
        typename SourceType::TFactorSlice>(SourceType::Slice).template To<SourceType>();
}

template <typename CodeGen, typename SliceType>
bool TryToGetFactorsSource(SliceType slice, IFactorsSource<CodeGen>*& source) {
    using TRegistry = ::NFactorsSourcePrivate::TFactorsSrcRegistry<CodeGen, SliceType>;

    return TRegistry::Instance().Lookup(slice, source);
}

template <typename SourceType>
bool TryToGetFactorsSource(SourceType*& source) {
    IFactorsSource<typename SourceType::TCodeGenInput>* sourceIf;
    bool ok = TryToGetFactorsSource<typename SourceType::TCodeGenInput,
        typename SourceType::TFactorSlice>(SourceType::Slice, sourceIf);
    if (ok) {
        source = &sourceIf->template To<SourceType>();
        return true;
    }
    return false;
}

#define REGISTER_FACTORS_SOURCE(SourceType, Id) \
    static ::NFactorsSourcePrivate::TRegisterSourceHelper<SourceType> regSource##Id;
