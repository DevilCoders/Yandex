#include "remap.h"

#include "request_accessors.h"
#include "reqbundle_accessors.h"

#include <library/cpp/json/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/list.h>

#include <kernel/reqbundle/scheme/options.sc.h>

namespace {
    using namespace NReqBundle;

    using TValueRemapSchemeConst = NReqBundleScheme::TValueRemapSchemeConst<TJsonTraits>;
    using TValueRemapScheme = NReqBundleScheme::TValueRemapScheme<TJsonTraits>;
    using TRemapTable = TMap<TFacetId, TDeque<TFacetRemap>>;

    THolder<TValueRemapSchemeConst> ParseValidate(
        const NJson::TJsonValue& value)
    {
        THolder<TValueRemapSchemeConst> scheme = MakeHolder<TValueRemapSchemeConst>(&value);

        scheme->Validate("", true, [&value](const TString& /*path*/, const TString& err){
            ythrow yexception()
                << "failed to parse JSON value '" << value << "'"
                << "\n" << err;
        });

        return scheme;
    }

    THolder<TRenormFracRemap> TryParseRenormFrac(
        const TValueRemapSchemeConst& scheme)
    {
        if (scheme.HasRenormFrac()) {
            const auto& paramsScheme = scheme.RenormFrac();
            THolder<TRenormFracRemap> renormFrac = MakeHolder<TRenormFracRemap>();
            renormFrac->Scale = paramsScheme.Scale();
            renormFrac->Offset = paramsScheme.Offset();
            renormFrac->Center = paramsScheme.Center();
            return renormFrac;
        }

        return {};
    }

    THolder<TConstValRemap> TryParseConstVal(
        const TValueRemapSchemeConst& scheme)
    {
        if (scheme.HasConstVal()) {
            const auto& paramsScheme = scheme.ConstVal();
            THolder<TConstValRemap> constVal = MakeHolder<TConstValRemap>();
            constVal->Value = paramsScheme.Value();
            return constVal;
        }

        return {};
    }

    THolder<TQuantizeRemap> TryParseQuantize(
        const TValueRemapSchemeConst& scheme)
    {
        if (scheme.HasQuantize()) {
            const auto& paramsScheme = scheme.Quantize();
            THolder<TQuantizeRemap> quantizeVal = MakeHolder<TQuantizeRemap>();
            quantizeVal->N = paramsScheme.N();
            return quantizeVal;
        }

        return {};
    }

    THolder<TClipRemap> TryParseClip(
        const TValueRemapSchemeConst& scheme)
    {
        if (scheme.HasClip()) {
            const auto& paramsScheme = scheme.Clip();
            THolder<TClipRemap> clipVal = MakeHolder<TClipRemap>();
            clipVal->MinValue = paramsScheme.MinValue();
            clipVal->MaxValue = paramsScheme.MaxValue();
            return clipVal;
        }

        return {};
    }

    void ParseRemap(
        const TValueRemapSchemeConst& scheme,
        TValueRemapPtr& res)
    {
        THolder<IValueRemap> result;

        if (auto holder = TryParseRenormFrac(scheme)) {
            result = std::move(holder);
        }
        if (auto holder = TryParseConstVal(scheme)) {
            Y_ENSURE(
                !result,
                "only one remap per clause is allowed"
            );

            result = std::move(holder);
        }
        if (auto holder = TryParseQuantize(scheme)) {
            Y_ENSURE(
                !result,
                "only one remap per clause is allowed"
            );

            result = std::move(holder);
        }
        if (auto holder = TryParseClip(scheme)) {
            Y_ENSURE(
                !result,
                "only one remap per clause is allowed"
            );

            result = std::move(holder);
        }

        Y_ENSURE(
            result,
            "failed to parse remap from value"
        );

        res = result.Release();
    }

    void ParseFacetRemap(
        const TValueRemapSchemeConst& scheme,
        TFacetRemap& res)
    {
        res.Facet = FacetIdFromJson(*(scheme.Facet().GetRawValue()));
        ParseRemap(scheme, res.Remap);
    }

    void SaveRemap(
        const TValueRemapPtr& remap,
        TValueRemapScheme& scheme)
    {
        *(scheme.GetRawValue()) = remap->ToJson();
    }

    void SaveFacetRemap(
        const TFacetRemap& remap,
        TValueRemapScheme& scheme)
    {
        SaveRemap(remap.Remap, scheme);
        *(scheme.Facet().GetRawValue()) = FacetIdToJson(remap.Facet);
    }

    void AddFacetIds(TConstRequestAcc request, TSet<TFacetId>& ids) {
        for (auto entry:  request.GetFacets().GetEntries()) {
            ids.insert(entry.GetId());
        }
    }

    void BuildRemapTable(
        const TSet<TFacetId>& ids,
        const TAllRemapOptions& opts,
        TRemapTable& remapTable)
    {
        for (const TFacetId& id : ids) {
            remapTable[id] = opts.FindAllApplicable(id);
        }
    }

    void RemapValuesByTable(
        const TRemapTable& table,
        TRequestAcc request)
    {
        for (auto entry : request.Facets().Entries()) {
            if (auto ptr = table.FindPtr(entry.GetId())) {
                for (const TFacetRemap& remap : *ptr) {
                    entry.SetValue(remap.Remap->Ku(entry.GetValue()));
                }
            }
        }
    }
} // namespace

namespace NReqBundle {
    THolder<TConstValRemap> TConstValRemap::TryFromJson(
        const TExplicitType<NJson::TJsonValue>& value)
    {
        return TryParseConstVal(
            *ParseValidate(value));
    }

    NJson::TJsonValue TConstValRemap::ToJson() const {
        NJson::TJsonValue value{NJson::JSON_MAP};
        TValueRemapScheme scheme(&value);

        auto&& paramsScheme = scheme.ConstVal();
        paramsScheme.Value() = Value;

        return value;
    }

    THolder<TRenormFracRemap> TRenormFracRemap::TryFromJson(
        const TExplicitType<NJson::TJsonValue>& value)
    {
        return TryParseRenormFrac(
            *ParseValidate(value));
    }

    NJson::TJsonValue TRenormFracRemap::ToJson() const {
        NJson::TJsonValue value{NJson::JSON_MAP};
        TValueRemapScheme scheme(&value);

        auto&& paramsScheme = scheme.RenormFrac();
        paramsScheme.Scale() = Scale;
        paramsScheme.Offset() = Offset;
        paramsScheme.Center() = Center;

        return value;
    }

    THolder<TQuantizeRemap> TQuantizeRemap::TryFromJson(
        const TExplicitType<NJson::TJsonValue>& value)
    {
        return TryParseQuantize(
            *ParseValidate(value));
    }

    NJson::TJsonValue TQuantizeRemap::ToJson() const {
        NJson::TJsonValue value{NJson::JSON_MAP};
        TValueRemapScheme scheme(&value);

        auto&& paramsScheme = scheme.Quantize();
        paramsScheme.N() = N;

        return value;
    }

    THolder<TClipRemap> TClipRemap::TryFromJson(
        const TExplicitType<NJson::TJsonValue>& value)
    {
        return TryParseClip(
            *ParseValidate(value));
    }

    NJson::TJsonValue TClipRemap::ToJson() const {
        NJson::TJsonValue value{NJson::JSON_MAP};
        TValueRemapScheme scheme(&value);

        auto&& paramsScheme = scheme.Clip();
        paramsScheme.MinValue() = MinValue;
        paramsScheme.MaxValue() = MaxValue;

        return value;
    }

    void TFacetRemap::FromJson(
        const TExplicitType<NJson::TJsonValue>& value)
    {
        ParseFacetRemap(
            *ParseValidate(value),
            *this);
    }

    NJson::TJsonValue TFacetRemap::ToJson() const {
        NJson::TJsonValue value{NJson::JSON_MAP};
        TValueRemapScheme scheme(&value);

        SaveFacetRemap(*this, scheme);
        return value;
    }

    TAllRemapOptions& TAllRemapOptions::FromJson(
        const TExplicitType<NJson::TJsonValue>& explicitValue)
    {
        const NJson::TJsonValue& value = explicitValue.Value();

        clear();

        if (value.IsNull()) {
            return *this;
        }

        if (value.IsMap()) {
            Add().FromJson(value);
            return *this;
        }

        for (const NJson::TJsonValue& itemValue : value.GetArraySafe()) {
            Add().FromJson(itemValue);
        }

        return *this;
    }

    TAllRemapOptions& TAllRemapOptions::FromJsonString(TStringBuf text) {
        NSc::TValue value = NSc::TValue::FromJson(text);
        FromJson(value.ToJsonValue());
        return *this;
    }

    NJson::TJsonValue TAllRemapOptions::ToJson() const {
        NJson::TJsonValue value{NJson::JSON_ARRAY};

        for (auto& entry : *this) {
            value.AppendValue(entry.ToJson());
        }

        return value;
    }

    TDeque<TValueRemapPtr> TAllRemapOptions::FindAll(const TFacetId& id) const {
        TDeque<TValueRemapPtr> res;

        for (const auto& entry : *this) {
            if (entry.Facet == id) {
                res.push_back(entry.Remap);
            }
        }

        return res;
    }

    TDeque<TFacetRemap> TAllRemapOptions::FindAllApplicable(const TFacetId& id) const {
        TDeque<TFacetRemap> res;

        for (const auto& entry : *this) {
            if (NStructuredId::IsSubId(id, entry.Facet, TIsSubRegion{})) {
                res.push_back(entry);
            }
        }

        return res;
    }

    void RemapValues(
        TRequestAcc request,
        const TAllRemapOptions& opts)
    {
        TSet<TFacetId> ids;
        AddFacetIds(request, ids);

        TRemapTable table;
        BuildRemapTable(ids, opts, table);

        RemapValuesByTable(table, request);
    }

    void RemapValues(
        TReqBundleAcc bundle,
        const TAllRemapOptions& opts)
    {
        TSet<TFacetId> ids;
        for (auto request : bundle.GetRequests()) {
            AddFacetIds(request, ids);
        }

        TRemapTable table;
        BuildRemapTable(ids, opts, table);

        for (auto request : bundle.Requests()) {
            RemapValuesByTable(table, request);
        }
    }
} // NReqBundle
