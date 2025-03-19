#include "mn_multi_categ.h"

#include <library/cpp/digest/md5/md5.h>

#include <util/generic/algorithm.h>
#include <util/ysaveload.h>
#include <util/stream/mem.h>
#include <util/stream/multi.h>

#include <cmath> // for exp()

namespace {
class TMD5Input: public IInputStream {
public:
    inline TMD5Input(IInputStream* slave) noexcept
        : Slave_(slave)
    {
    }

    ~TMD5Input() override {
    }

    inline TString ToString() {
        char md5buf[33];
        return TString(MD5Sum.End(md5buf));
    }

private:
    size_t DoRead(void* buf, size_t len) override {
        const size_t ret = Slave_->Read(buf, len);
        MD5Sum.Update(buf, ret);
        return ret;
    }

    /* Note that default implementation of DoSkip works perfectly fine here as
     * it's implemented in terms of DoRead. */

private:
    IInputStream* Slave_;
    MD5 MD5Sum;
};
} // namespace

namespace NMatrixnet {

class TMnMultiCategImpl {
public:
    virtual void Save(IOutputStream *out) const = 0;
    virtual void Load(IInputStream *in) = 0;
    virtual const TString& MD5() const = 0;
    /** Calculate values for all possible categories for each document.
     *
     * docsFactors -- vector where each element is a document (factors plane)
     * result -- 2-dimensional array with size docs_factors.size() * CategValues().size()
     *          i*CategValues().size() + j is value of j-th category for i-th document
     */
    void CalcMulti(const TVector<TVector<float>>& docsFactors, double* result) {
        TVector<const float*> planePtrs(docsFactors.size());
        for (size_t i = 0; i < docsFactors.size(); ++i) {
            planePtrs[i] = docsFactors[i].data();
        }
        CalcMulti(planePtrs.data(), result, docsFactors.size());
    }
    virtual void CalcMulti(const float* const* docsFactors, double* result, const size_t numDocs) = 0;
    virtual int NumFeatures() const = 0;
    virtual size_t GetNumFeats() const = 0;
    virtual void UsedFactors(TSet<ui32>& factors) const = 0;
    virtual void UsedFactors(TSet<NFactorSlices::TFullFactorIndex>& factors) const = 0;
    virtual ~TMnMultiCategImpl() {}
    virtual THolder<TMnMultiCategImpl> Clone() const = 0;

    virtual void SetInfo(const TString& key, const TString& value) = 0;
    virtual const TModelInfo* GetInfo() const = 0;

    virtual TConstArrayRef<double> GetClassValues() const = 0;
};

class TMultivalueSseModelImpl : public TMnMultiCategImpl {
    mutable TString MD5Sum;
    struct TSolidState {
        TMnSseModel MutlivalueModel;
        TVector<double> ClassValues;
    };
    struct TThinState {
        TMnSseInfo MutlivalueModel;
        TConstArrayRef<double> ClassValues;
    };
    std::variant<TSolidState, TThinState> State = TSolidState();

    TMnSseInfo& GetStateSseInfo() {
        if (std::holds_alternative<TSolidState>(State)) {
            return std::get<TSolidState>(State).MutlivalueModel;
        } else {
            return std::get<TThinState>(State).MutlivalueModel;
        }
    }
    const TMnSseInfo& GetStateSseInfo() const {
        if (std::holds_alternative<TSolidState>(State)) {
            return std::get<TSolidState>(State).MutlivalueModel;
        } else {
            return std::get<TThinState>(State).MutlivalueModel;
        }
    }
public:
    TMultivalueSseModelImpl() = default;
    TMultivalueSseModelImpl(const TMultivalueSseModelImpl& other) = default;
    TMultivalueSseModelImpl(const TVector<TMnSseDynamicPtr>& models, TVector<double>&& classValues)
    {
        auto& stateRef = std::get<TSolidState>(State);
        stateRef.ClassValues = std::move(classValues);
        Y_ENSURE(stateRef.ClassValues.size() == models.size());
        Y_ENSURE(models.size() > 0);

        auto sseData0 = models[0]->GetSseDataPtrs();
        TMnSseStaticMeta commonMeta = sseData0.Meta;
        TMultiData mergedMultiDatas(models.size(), std::get<TMultiData>(sseData0.Leaves.Data).DataSize);

        for (auto model : models) {
            auto sseData = model->GetSseDataPtrs();
            const auto& modelMultiData = std::get<TMultiData>(sseData.Leaves.Data);

            Y_ENSURE(commonMeta.CompareValues(sseData.Meta));
            Y_ENSURE(modelMultiData.MultiData.size() == 1);
            Y_ENSURE(modelMultiData.DataSize == mergedMultiDatas.DataSize);

            TMultiData::TLeafData leaves = modelMultiData.MultiData[0];
            mergedMultiDatas.MultiData.push_back(leaves);
        }

        stateRef.MutlivalueModel.CopyFrom(commonMeta, TMnSseStaticLeaves(mergedMultiDatas));
    }

    TMultivalueSseModelImpl(const void* const buf, const size_t size) {
        InitFromBuf(buf, size);
    }

    void InitFromBuf(const void* const buf, const size_t size) {
        State = TThinState();
        auto& state = std::get<TThinState>(State);
        TMemoryInput memInput(buf, size);
        ui32 marker = 0;
        ::Load(&memInput, marker);
        Y_ENSURE(FLATBUFFERS_MNMC_MODEL_MARKER == marker, "Trying to initialize TMultivalueSseModelImpl on corrupted data");
        const char* modelBeginPtr = memInput.Buf();
        ::Load(&memInput, marker);
        Y_ENSURE(FLATBUFFERS_MN_MODEL_MARKER == marker, "Trying to initialize TMultivalueSseModelImpl on corrupted data");
        ui32 sseFlatbufModelLen = 0;
        ::Load(&memInput, sseFlatbufModelLen);
        Y_ENSURE(sseFlatbufModelLen < memInput.Avail(), "Insufficient buffer size");
        // flatbuf mn model has ui32 with model marker and ui32 with flatbuf message length
        constexpr auto flatbufModelPreambleSize = sizeof(ui32) * 2;
        state.MutlivalueModel.InitStatic(modelBeginPtr, flatbufModelPreambleSize + static_cast<size_t>(sseFlatbufModelLen));
        memInput.Skip(sseFlatbufModelLen);
        ui32 classesArraySize = 0;
        ::Load(&memInput, classesArraySize);
        Y_ENSURE(classesArraySize * 8 <= memInput.Avail());
        state.ClassValues = MakeArrayRef((double*)memInput.Buf(), classesArraySize);
    }

    TMultivalueSseModelImpl(const TMnSseModel& other, const TVector<double>& classValues)
    {
        State = TSolidState{other, classValues};
    }

    ~TMultivalueSseModelImpl() = default;

    THolder<TMnMultiCategImpl> Clone() const override {
        return MakeHolder<TMultivalueSseModelImpl>(*this);
    }

    void Save(IOutputStream *out) const override {
        Y_ENSURE(std::holds_alternative<TSolidState>(State), "Thin multiclass model serialization unsupported");
        auto& state = std::get<TSolidState>(State);
        ::Save(out, FLATBUFFERS_MNMC_MODEL_MARKER);
        ::Save(out, state.MutlivalueModel);
        ::Save(out, state.ClassValues);
    }
    void Load(IInputStream *rawIn) override {
        State = TSolidState();
        auto& state = std::get<TSolidState>(State);
        TMD5Input in(rawIn);
        ui32 formatDescriptor = 0;
        ::Load(&in, formatDescriptor);
        Y_ENSURE(formatDescriptor == FLATBUFFERS_MNMC_MODEL_MARKER);
        ::Load(&in, state.MutlivalueModel);
        ::Load(&in, state.ClassValues);
        MD5Sum = in.ToString();
    }
    const TString& MD5() const override {
        if (MD5Sum.empty()) {
            ::MD5 md5;
            auto modelMD5 = GetStateSseInfo().MD5();
            md5.Update(modelMD5.data(), modelMD5.size());
            char md5buf[33];
            MD5Sum = md5.End(md5buf);
        }
        return MD5Sum;
    }

    void CalcMulti(const float* const* docsFactors, double* result, const size_t numDocs) override {
        GetStateSseInfo().DoCalcMultiDataRelevs(docsFactors, result, numDocs, 1, GetClassValues().size());
    }

    int NumFeatures() const override {
        return GetStateSseInfo().NumFeatures();
    }

    size_t GetNumFeats() const override {
        return GetStateSseInfo().GetNumFeats();
    }

    void UsedFactors(TSet<ui32>& factors) const override {
        GetStateSseInfo().UsedFactors(factors);
    }

    void UsedFactors(TSet<NFactorSlices::TFullFactorIndex>& factors) const override {
        GetStateSseInfo().UsedFactors(factors);
    }

    void SetInfo(const TString& key, const TString& value) override {
        GetStateSseInfo().SetInfo(key, value);
    }

    const TModelInfo* GetInfo() const override {
        return GetStateSseInfo().GetInfo();
    }

    TConstArrayRef<double> GetClassValues() const override {
        if (std::holds_alternative<TSolidState>(State)) {
            return std::get<TSolidState>(State).ClassValues;
        } else {
            return std::get<TThinState>(State).ClassValues;
        }
    }
};

class TMnMultiCategImplOld : public TMnMultiCategImpl {
    TVector<TMnSseDynamicPtr> DynamicMns;
    TVector<double> ClassValues;
    TModelInfo ModelInfo;
    mutable TString MD5Sum;

public:
    TMnMultiCategImplOld() = default;
    TMnMultiCategImplOld(const TMnMultiCategImplOld& other)
        : TMnMultiCategImpl(other)
        , ModelInfo(other.ModelInfo)
    {
        for (auto& model : other.DynamicMns) {
            DynamicMns.push_back(MakeAtomicShared<TMnSseDynamic>(*model));
        }
    }
    TMnMultiCategImplOld(TVector<TMnSseDynamicPtr>&& models, TVector<double>&& classValues)
        : DynamicMns(std::move(models))
    {
        ClassValues = std::move(classValues);
    }

    ~TMnMultiCategImplOld() = default;

    THolder<TMnMultiCategImpl> Clone() const override {
        return MakeHolder<TMnMultiCategImplOld>(*this);
    }

    void Save(IOutputStream *out) const override {
        ::Save(out, ClassValues);
        for (size_t i = 0; i < DynamicMns.size(); ++i) {
            ::Save(out, *DynamicMns[i]);
        }
        ::Save(out, ModelInfo);
    }
    void Load(IInputStream *in) override {
        ::Load(in, ClassValues);

        DynamicMns.clear();
        for (size_t i = 0; i < ClassValues.size(); ++i) {
            THolder<TMnSseDynamic> mn(new TMnSseDynamic());
            ::Load(in, *mn);
            DynamicMns.push_back(TMnSseDynamicPtr(mn.Release()));
        }

        try {
            ::Load(in, ModelInfo);
        } catch (const TLoadEOF&) {
            // that's normal. Just loading model in old format without props field.
        }

    }
    const TString& MD5() const override {
        if (MD5Sum.empty() && DynamicMns.size()) {
            ::MD5 md5;
            for (const auto& model : DynamicMns) {
                md5.Update(model->MD5().data(), model->MD5().size());
            }
            char md5buf[33];
            MD5Sum = md5.End(md5buf);
        }
        return MD5Sum;
    }

    void CalcMulti(const float* const* docsFactors, double* result, const size_t numDocs) override {
        TVector<double> tmpRes(numDocs);
        const auto classCount = DynamicMns.size();
        for (size_t classId = 0; classId < classCount; ++classId) {
            DynamicMns[classId]->DoCalcRelevs(docsFactors, tmpRes.data(), numDocs);
            for (size_t docId = 0; docId < numDocs; ++docId) {
                result[docId * classCount + classId] = tmpRes[docId];
            }
        }
    }

    int NumFeatures() const override {
        int num = 0;
        for (const auto& mn : DynamicMns) {
            num = Max(num, mn->NumFeatures());
        }
        return num;
    }

    size_t GetNumFeats() const override {
        size_t max = 0;
        for (const auto& mn : DynamicMns) {
            max = Max(max, mn->GetNumFeats());
        }
        return max;
    }

    void UsedFactors(TSet<ui32>& factors) const override {
        factors.clear();
        TSet<ui32> tmpFactors;
        for (const auto& mn : DynamicMns) {
            mn->UsedFactors(tmpFactors);
            factors.insert(tmpFactors.begin(), tmpFactors.end());
        }
    }
    void UsedFactors(TSet<NFactorSlices::TFullFactorIndex>& factors) const override {
        factors.clear();
        TSet<NFactorSlices::TFullFactorIndex> tmpFactors;
        for (const auto& mn : DynamicMns) {
            mn->UsedFactors(tmpFactors);
            factors.insert(tmpFactors.begin(), tmpFactors.end());
        }
    }

    void SetInfo(const TString& key, const TString& value) override {
        ModelInfo[key] = value;
    }

    const TModelInfo* GetInfo() const override {
        return &ModelInfo;
    }

    TConstArrayRef<double> GetClassValues() const override {
        return ClassValues;
    }
};

TMnMultiCateg::TMnMultiCateg() {
    Implementation = MakeHolder<TMultivalueSseModelImpl>();
}

TMnMultiCateg::TMnMultiCateg(const void* const buf, const size_t size, const char* const /* name */) {
    TMemoryInput in(buf, size);
    ui32 formatDescriptor = 0;
    ::Load(&in, formatDescriptor);
    if (formatDescriptor != FLATBUFFERS_MNMC_MODEL_MARKER) {
        TMemoryInput input(buf, size);
        Implementation = MakeHolder<TMnMultiCategImplOld>();
        Implementation->Load(&input);
    } else {
        Implementation = MakeHolder<TMultivalueSseModelImpl>(buf, size);
    }
}

TMnMultiCateg::TMnMultiCateg(const TMnMultiCateg& other) {
    Implementation = other.Implementation->Clone();
}

TMnMultiCateg::TMnMultiCateg(THolder<TMnMultiCategImpl> impl)
        : Implementation(impl.Release())
{}

TMnMultiCateg::TMnMultiCateg(TVector<TMnSseDynamicPtr>&& models, TVector<double>&& classValues)
        : Implementation(MakeHolder<TMultivalueSseModelImpl>(std::move(models), std::move(classValues)))
{}

/*
 * Multi-category matrixnet model for blender softmax formulae that allow compact representation.
 * In blender mnmc softmax formulae i-th leaf always contains predict delta for only one class.  Thus we can
 * use the same TMnSseStatic::TData, but additionally store leaf->class mapping.
 *
 * leaf   class1  class2  class3        delta  class#
 *
 *   0     d0c1    d0c2    d0c3           d0    cn0      // \forall i!=cn0 => d0c{i} == 0
 *   1     d1c1    d1c2    d1c3           d1    cn1
 *   2     d2c1    d2c2    d2c3    ->     d2    cn2
 *   3     d3c1    d3c2    d3c3           d3    cn3
 *   .
 *   .
 *   .
 *  63     d63c1   d63c2   d63c3          d63   cn63
 *
 * See https://st.yandex-team.ru/BLNDR-1781 for discussion
 *
 * Use mx_ops compress_mc to convert .bin mnmc model to compact mnmc.
 */
THolder<TMnMultiCateg> MakeCompactMultiCateg(TVector<TMnSseDynamicPtr>&& models, TVector<double>&& classValues) {
    Y_ENSURE(classValues.size() == models.size());
    Y_ENSURE(classValues.size() <= MAX_COMPACT_MODEL_CLASSES);
    Y_ENSURE(models.size() > 0);

    auto sseData0 = models[0]->GetSseDataPtrs();

    TMnSseStaticMeta commonMeta = sseData0.Meta;
    size_t commonDataSize = std::get<TMultiData>(sseData0.Leaves.Data).DataSize;
    TNormAttributes commonNormAttributes = std::get<TMultiData>(sseData0.Leaves.Data).MultiData.front().Norm;

    Y_ENSURE(commonNormAttributes.DataBias == 0);
    Y_ENSURE(commonNormAttributes.NativeDataBias == 0);
    for (const TMnSseDynamicPtr& model: models) {
        auto sseData = model->GetSseDataPtrs();
        const TMultiData& modelMultiData = std::get<TMultiData>(sseData.Leaves.Data);
        Y_ENSURE(modelMultiData.MultiData.size() == 1);
        Y_ENSURE(commonMeta.CompareValues(model->GetSseDataPtrs().Meta));
        Y_ENSURE(commonDataSize == modelMultiData.DataSize);
        Y_ENSURE(commonNormAttributes.Compare(modelMultiData.MultiData.front().Norm));
    }

    TVector<TValueClassLeaf> unifiedData;
    size_t dataOffset = 0;
    const int ZERO = 1 << 31;
    for (size_t treeSize = 0; treeSize <= 8; ++treeSize) {
        size_t treesOfSize = size_t(sseData0.Meta.GetSizeToCount(treeSize));
        for (size_t treeId = 0; treeId < treesOfSize; ++treeId) {
            for (size_t leafId = 0; leafId < (1u << treeSize); ++leafId) {
                size_t nonZeroCount = 0;
                for (size_t classId: xrange(models.size())) {
                    int leafValue = std::get<TMultiData>(models[classId]->GetSseDataPtrs().Leaves.Data).MultiData[0].Data[dataOffset];
                    if (leafValue != ZERO) {
                        Y_ENSURE(classId <= size_t(std::numeric_limits<ui8>::max()));
                        unifiedData.push_back(TValueClassLeaf(leafValue, ui8(classId)));
                        ++nonZeroCount;
                    }
                }

                Y_ENSURE(nonZeroCount <= 1);
                if (nonZeroCount == 0) {
                    unifiedData.push_back(TValueClassLeaf(ZERO, 0));
                }

                ++dataOffset;
            }
        }
    }
    Y_ENSURE(dataOffset == unifiedData.size());

    TMultiDataCompact compact(unifiedData.data(), commonDataSize, commonNormAttributes, models.size());

    TMnSseModel model;
    model.CopyFrom(commonMeta, TMnSseStaticLeaves(compact));

    return MakeHolder<TMnMultiCateg>(MakeHolder<TMultivalueSseModelImpl>(model, std::move(classValues)));
}

void TMnMultiCateg::Swap(TMnMultiCateg &obj) {
    DoSwap(Implementation, obj.Implementation);
}


TConstArrayRef<double> TMnMultiCateg::CategValues() const {
    return Implementation->GetClassValues();
}

void TMnMultiCateg::Save(IOutputStream *out) const {
    Implementation->Save(out);
}

void TMnMultiCateg::Load(IInputStream *in) {
    ui32 formatDescriptor = 0;
    ::Load(in, formatDescriptor);
    if (formatDescriptor != FLATBUFFERS_MNMC_MODEL_MARKER) {
        Implementation = MakeHolder<TMnMultiCategImplOld>();
    } else {
        Implementation = MakeHolder<TMultivalueSseModelImpl>();
    }
    TMemoryInput memInp(&formatDescriptor, sizeof(ui32));
    TMultiInput multiInp(&memInp, in);
    Implementation->Load(&multiInp);
}

void TMnMultiCateg::CalcCategs(const TVector< TVector<float> >& docsFactors, double* resultCategValues) const {
    const size_t numDocs = docsFactors.size();
    const auto numCategs = Implementation->GetClassValues().size();
    TVector<double> curResult(numDocs * numCategs);
    Implementation->CalcMulti(docsFactors, curResult.data());
    for (size_t docId = 0; docId < numDocs; ++docId) {
        double sum = 0.0;
        for (size_t categId = 0; categId < numCategs; ++categId) {
            const double expValue = exp(curResult[docId * numCategs + categId]);
            resultCategValues[docId * numCategs + categId] = expValue;
            sum += expValue;
        }
        for (size_t categId = 0; categId < numCategs; ++categId) {
            resultCategValues[docId*numCategs + categId] /= sum;
        }
    }
}

void TMnMultiCateg::CalcCategoriesRanking(const TVector<TVector<float>> &docsFactors, double* resultCategValues) const {
    Implementation->CalcMulti(docsFactors, resultCategValues);
}

int TMnMultiCateg::NumFeatures() const {
    return Implementation->NumFeatures();
}

void TMnMultiCateg::UsedFactors(TSet<ui32>& factors) const {
    Implementation->UsedFactors(factors);
}

void TMnMultiCateg::UsedFactors(TSet<NFactorSlices::TFullFactorIndex>& factors) const {
    Implementation->UsedFactors(factors);
}

double TMnMultiCateg::DoCalcRelev(const float* factors) const {
    double result = 0.0;
    DoCalcRelevs(&factors, &result, 1);
    return result;
}

void TMnMultiCateg::DoCalcRelevs(const float* const* docsFactors, double* resultRelev, const size_t numDocs) const {
    for (size_t i = 0; i < numDocs; ++i) {
        resultRelev[i] = 0.0;
    }
    const auto classValues = Implementation->GetClassValues().data();
    const size_t numCategs = Implementation->GetClassValues().size();
    TVector<double> cur(numDocs * numCategs, 0.0);
    Implementation->CalcMulti(docsFactors, cur.data(), numDocs);
    for (size_t docId = 0; docId < numDocs; ++docId) {
        double sum = 0.0;
        for (size_t categId = 0; categId < numCategs; ++categId) {
            const double expValue = exp(cur[docId * numCategs + categId]);
            sum += expValue;
            resultRelev[docId] += expValue * classValues[categId];
        }
        resultRelev[docId] /= sum;
    }
}

size_t TMnMultiCateg::GetNumFeats() const {
    return Implementation->GetNumFeats();
}

const TString& TMnMultiCateg::MD5() const {
    return Implementation->MD5();
}

TMnMultiCateg::~TMnMultiCateg() {}

void TMnMultiCateg::SetInfo(const TString& key, const TString& value) {
    Implementation->SetInfo(key, value);
}

const TModelInfo* TMnMultiCateg::GetInfo() const {
    return Implementation->GetInfo();
}

} // namespace NMatrixnet
