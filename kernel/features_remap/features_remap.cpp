#include "features_remap.h"
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <kernel/factor_slices/slices_info.h>
#include <kernel/factor_storage/factor_storage.h>
#include <library/cpp/protobuf/util/pb_io.h>
#include <library/cpp/expression/expression.h>
#include <util/stream/mem.h>

using namespace NFeaturesRemap;

NProto::TCalculations NFeaturesRemap::ParseTxt(TStringBuf in) {
    NProto::TCalculations result;
    TMemoryInput stream(in.begin(), in.size());
    ParseFromTextFormat(stream, result);
    return result;
}

NProto::TCalculations NFeaturesRemap::ParseCgi(TStringBuf in, bool escaped) {
    NProto::TCalculations result;

    TString state{in};
    if (!escaped) {
        CGIUnescape(state);
    }
    state = Base64StrictDecode(state);
    Y_PROTOBUF_SUPPRESS_NODISCARD result.ParseFromString(state);

    return result;
}

namespace {
    void RestoreHrFeatureInfo(NProto::TFeatutreDescr& obj) {
        NFactorSlices::EFactorSlice slice = FromString(obj.GetSlice());
        auto sliceInfoPtr = NFactorSlices::GetSlicesInfo()->GetFactorsInfo(slice);
        Y_ENSURE(sliceInfoPtr);
        Y_ENSURE(obj.HasId());
        TString name = sliceInfoPtr->GetFactorName(obj.GetId());

        if (obj.HasName()) {
            Y_ENSURE(name == obj.GetName());
        }
        if (name) {
            obj.SetName(name);
        }
    }

    void ClearHrFeatureInfo(NProto::TFeatutreDescr& obj) {
        NFactorSlices::EFactorSlice slice = FromString(obj.GetSlice());
        auto sliceInfoPtr = NFactorSlices::GetSlicesInfo()->GetFactorsInfo(slice);
        Y_ENSURE(sliceInfoPtr);

        if (obj.HasName()) {
            auto id = sliceInfoPtr->GetFactorIndex(obj.GetName().c_str());
            Y_ENSURE(id, "unknown feature: '" << obj.GetName() << "'");
            if (obj.HasId()) {
                Y_ENSURE(*id == obj.GetId(), "correct id is: " << *id);
            }
            obj.SetId(*id);
        }
        obj.ClearName();
        Y_ENSURE(obj.HasId());
    }
}

void NFeaturesRemap::RestoreFullInfo(NProto::TCalculations& in) {
    for(auto& s : *in.MutableStageCalculation()) {
        for(auto& x : *s.MutableLet()) {
            RestoreHrFeatureInfo(*x.MutableFeatureSrc());
        }
        for(auto& x : *s.MutableSet()) {
            RestoreHrFeatureInfo(*x.MutableFeatureDst());
        }
    }
}

void NFeaturesRemap::ClearHrInfo(NProto::TCalculations& in) {
    for(auto& s : *in.MutableStageCalculation()) {
        for(auto& x : *s.MutableLet()) {
            ClearHrFeatureInfo(*x.MutableFeatureSrc());
        }
        for(auto& x : *s.MutableSet()) {
            ClearHrFeatureInfo(*x.MutableFeatureDst());
        }
    }
}

TString NFeaturesRemap::ToCgiAsIs(const NProto::TCalculations& in, bool escaped) {
    TString result;
    Y_PROTOBUF_SUPPRESS_NODISCARD in.SerializeToString(&result);
    result = Base64Encode(result);
    if (escaped) {
        CGIEscape(result);
    }
    return result;
}

TString NFeaturesRemap::ToCgiAutoConvertations(NProto::TCalculations& in, bool escaped) {
    ClearHrInfo(in);
    return ToCgiAsIs(in, escaped);
}

namespace {
    class TCalulationContext {
    private:
        const NProto::TCalculations& Program;
        NProto::EStage StageFilter;
        TStringBuf SearchType;
        THashMap<TString, double> AdditionalContext;
        TCalcApplyReport Report;
        bool ErrorIsTruncated = false;
    public:
        TCalulationContext(
            const NProto::TCalculations& program,
            NProto::EStage stageFilter,
            TStringBuf searchType,
            THashMap<TString, double>&& additionalContext
        )
            : Program(program)
            , StageFilter(stageFilter)
            , SearchType(searchType)
            , AdditionalContext(std::move(additionalContext))
        {}

        void HandleView(TFactorView& view) {
            HandleViewImpl(view, [](TFullFactorIndex /*index*/){});
        }

        void HandleView(TFactorView& view, TVector<TFullFactorIndex>& dstFactors) {
            HandleViewImpl(view, [&](TFullFactorIndex index){ dstFactors.push_back(index); });
        }

        void CopyFactors(const TConstFactorView& srcView, TFactorView& dstView, const TVector<TFullFactorIndex>& factorIndexes) {
            try {
                for (auto index : factorIndexes) {
                    Y_ASSERT(srcView.Has(index));
                    Y_ENSURE(dstView.Has(index), "bad index: " << index.Slice << "[" << index.Index << "]");
                    dstView[index] = srcView[index];
                }
            } catch (const yexception& ex) {
                HandleException(ex);
            }
        }

        TCalcApplyReport MoveReport() {
            return std::move(Report);
        }

    private:
        template<typename Func>
        void HandleViewImpl(TFactorView& view, Func dstFactorAction) {
            try {
                for (const auto& c : Program.GetStageCalculation()) {
                    if (c.GetStage() != StageFilter) {
                        continue;
                    }
                    if (!c.GetSearchTypes().empty() && !SearchType.empty()
                        && std::count(c.GetSearchTypes().begin(), c.GetSearchTypes().end(), SearchType) == 0
                    ) {
                        continue;
                    }

                    auto curContext = AdditionalContext;
                    for(auto& l : c.GetLet()) {
                        HandleLet(l, view, curContext);
                    }

                    for(auto& s : c.GetSet()) {
                        HandleSet(s, view, curContext, dstFactorAction);
                    }

                    for (auto& ifItem : c.GetIf()) {
                        if (CalcExpression(ifItem.GetCondition(), curContext)) {
                            for (const auto& s : ifItem.GetSet()) {
                                HandleSet(s, view, curContext, dstFactorAction);
                            }
                        }
                    }
                }
            } catch (const yexception& ex) {
                HandleException(ex);
            }
        }

        static void HandleLet(const NProto::TInitDescr& l, const TFactorView& view, THashMap<TString, double>& context) {
            Y_ENSURE(l.HasFeatureSrc());
            Y_ENSURE(l.GetFeatureSrc().HasSlice());
            Y_ENSURE(l.GetFeatureSrc().HasId());
            TFullFactorIndex srcIndex(FromString(l.GetFeatureSrc().GetSlice()), l.GetFeatureSrc().GetId());
            Y_ENSURE(view.Has(srcIndex), "bad index: " << srcIndex.Slice << "[" << srcIndex.Index << "]");
            context[l.GetDefineVar()] = view[srcIndex];
        }

        template<typename Func>
        void HandleSet(const NProto::TCalcDescr& s, TFactorView& view, THashMap<TString, double>& context, Func dstFactorAction) {
            Y_ENSURE(s.HasFeatureDst());
            Y_ENSURE(s.GetFeatureDst().HasSlice());
            Y_ENSURE(s.GetFeatureDst().HasId());
            TFullFactorIndex dstIndex(FromString(s.GetFeatureDst().GetSlice()), s.GetFeatureDst().GetId());
            Y_ENSURE(view.Has(dstIndex), "bad index: " << dstIndex.Slice << "[" << dstIndex.Index << "]");
            view[dstIndex] = CalcExpression(s.GetExpression(), context);
            dstFactorAction(dstIndex);
            Report.DoneModifications = true;
        }

        void HandleException(const yexception& ex) {
            Report.HasErrors = true;
            if (Report.ErrorsReport.size() > 4000 && !ErrorIsTruncated) {
                Report.ErrorsReport += "<truncated>; execution stopped";
                ErrorIsTruncated = true;
            } else if (!ErrorIsTruncated) {
                Report.ErrorsReport += TString(ex.what()) + "\n";
            }
        }
    };
}


TCalcApplyReport NFeaturesRemap::DoCalculation(
    const NProto::TCalculations& c,
    TFactorView& view,
    NProto::EStage stageFilter,
    TStringBuf searchType,
    THashMap<TString, double>&& additionalContext
) {
    TCalulationContext context(c, stageFilter, searchType, std::move(additionalContext));
    context.HandleView(view);
    return context.MoveReport();
}

TCalcApplyReport NFeaturesRemap::DoCalculation(
    const NProto::TCalculations& c,
    const TVector<TFactorStorage*>& storages,
    NProto::EStage stageFilter,
    TStringBuf searchType,
    THashMap<TString, double>&& additionalContext
) {
    TCalulationContext context(c, stageFilter, searchType, std::move(additionalContext));
    bool isFirst = true;
    TVector<TFullFactorIndex> dstIndexes;
    for (TFactorStorage* storage : storages) {
        auto mutableView = storage->CreateView();
        if (c.GetAssumeThatAllVariablesAreConstantOverDocs()) {
            if (isFirst) {
                context.HandleView(mutableView, dstIndexes);
            } else {
                context.CopyFactors(storages.front()->CreateConstView(), mutableView, dstIndexes);
            }
        } else {
            context.HandleView(mutableView);
        }
        isFirst = false;
    }
    return context.MoveReport();
}
