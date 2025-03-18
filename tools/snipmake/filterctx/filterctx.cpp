#include <kernel/snippets/idl/snippets.pb.h>
#include <kernel/snippets/schemaorg/proto/schemaorg.pb.h>
#include <kernel/snippets/schemaorg/schemaorg_serializer.h>
#include <kernel/snippets/schemaorg/schemaorg_traversal.h>
#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/tarcface.h>
#include <kernel/tarc/markup_zones/text_markup.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/langmask/langmask.h>

#include <library/cpp/langs/langs.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/stream/output.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/string_utils/url/url.h>

using TSnippetsCtx = NSnippets::NProto::TSnippetsCtx;
using TArc = NSnippets::NProto::TArc;

bool ParseContext(const TString& sctx, TSnippetsCtx& ctx) {
    TString decoded;
    try {
        decoded = Base64Decode(sctx);
    } catch (yexception&) {
        return false;
    }
    return ctx.ParseFromString(decoded);
}

bool ParseDocDescr(const TSnippetsCtx& ctx, TDocDescr& docDescr) {
    if (!ctx.HasTextArc()) {
        return false;
    }
    const TArc& arc = ctx.GetTextArc();
    if (!arc.HasValid() || !arc.GetValid() || !arc.HasExtInfo()) {
        return false;
    }
    docDescr.UseBlob(arc.GetExtInfo().data(), arc.GetExtInfo().size());
    return true;
}

void ParseDocInfos(TDocDescr& docDescr, TDocInfos& docInfos) {
    docDescr.ConfigureDocInfos(docInfos);
}

bool ParseZones(const TSnippetsCtx& ctx, TArchiveMarkupZones& zones) {
    if (!ctx.HasTextArc()) {
        return false;
    }
    const TArc& arc = ctx.GetTextArc();
    if (!arc.HasValid() || !arc.GetValid() || !arc.HasData()) {
        return false;
    }
    const ui8* data = (const ui8*)arc.GetData().data();
    TArchiveTextHeader* hdr = (TArchiveTextHeader*)data;
    data += sizeof(TArchiveTextHeader);
    UnpackMarkupZones(data, hdr->InfoLen, &zones);
    return true;
}

bool ContainsMediawikiZone(const TSnippetsCtx& ctx, const TString& zoneFilter) {
    EArchiveZone zone = FromString(zoneFilter.data());
    if (zone == AZ_COUNT) {
        return false;
    }
    TArchiveMarkupZones zones;
    if (!ParseZones(ctx, zones)) {
        return false;
    }
    return !zones.GetZone(zone).Spans.empty();
}

bool ContainsAttr(const TSnippetsCtx& ctx, const TString& attrFilter) {
    TDocDescr docDescr;
    if (!ParseDocDescr(ctx, docDescr)) {
        return false;
    }
    TDocInfos docInfos;
    ParseDocInfos(docDescr, docInfos);
    return docInfos.contains(attrFilter.data());
}

bool ContainsHost(const TSnippetsCtx& ctx, const TString& hostFilter) {
    TDocDescr docDescr;
    if (!ParseDocDescr(ctx, docDescr)) {
        return false;
    }
    TString url = docDescr.get_url();
    TStringBuf host = GetOnlyHost(url);
    return host == hostFilter || (hostFilter.StartsWith('.') &&
        (TString(".") + host == hostFilter || host.EndsWith(hostFilter)));
}

bool ContainsUil(const TSnippetsCtx& ctx, const TString& uilFilter) {
    if (!ctx.HasReqParams() || !ctx.GetReqParams().HasUILang()) {
        return false;
    }
    return ctx.GetReqParams().GetUILang() == uilFilter;
}

bool ContainsDocLang(const TSnippetsCtx& ctx, const TString& docLangFilter) {
    TString docLang;
    TDocDescr docDescr;
    if (ParseDocDescr(ctx, docDescr)) {
        TDocInfos docInfos;
        ParseDocInfos(docDescr, docInfos);
        TDocInfos::const_iterator lang = docInfos.find("lang");
        if (lang != docInfos.end()) {
            docLang = lang->second;
        }
    }
    if (!docLang && ctx.HasReqParams() && ctx.GetReqParams().HasForeignNavHackLang()) {
        docLang = ctx.GetReqParams().GetForeignNavHackLang();
    }
    return !docLang && docLangFilter == "unknown" || docLang == docLangFilter;
}

bool ContainsQueryLang(const TSnippetsCtx& ctx, const TString& queryLangFilter) {
    const ELanguage langId = LanguageByName(queryLangFilter.data());
    if (!langId && queryLangFilter != "unknown") {
        return false;
    }
    if (!ctx.HasReqParams() || !ctx.GetReqParams().HasQueryLangMask()) {
        return false;
    }
    TLangMask langMask(ctx.GetReqParams().GetQueryLangMaskLow());
    return langMask.Empty() && !langId || langMask.Test(langId);
}

bool ContainsItemtype(const TSnippetsCtx& ctx, const TString& itemtype) {
    TString serializedSchemaOrg;
    TDocDescr docDescr;
    if (ParseDocDescr(ctx, docDescr)) {
        TDocInfos docInfos;
        ParseDocInfos(docDescr, docInfos);
        TDocInfos::const_iterator it = docInfos.find("SchemaOrg");
        if (it != docInfos.end()) {
            serializedSchemaOrg = it->second;
        }
    }
    if (serializedSchemaOrg) {
        NSchemaOrg::TTreeNode root;
        if (NSchemaOrg::DeserializeFromBase64(serializedSchemaOrg, root)) {
            for (size_t i = 0; i < root.NodeSize(); ++i) {
                if (IsItemOfType(root.GetNode(i), itemtype)) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool HasReportType(const TSnippetsCtx& ctx, const TString& reportTypeFilter) {
    if (!ctx.HasReqParams() || !ctx.GetReqParams().HasReport()) {
        return false;
    }
    return ctx.GetReqParams().GetReport() == reportTypeFilter;
}

bool IsMainPage(const TSnippetsCtx& ctx) {
    if (!ctx.HasReqParams() || !ctx.GetReqParams().HasMainPage()) {
        return false;
    }
    return ctx.GetReqParams().GetMainPage();
}

class TContextsFilter {
public:
    TString ZoneFilter;
    TString AttrFilter;
    TString HostFilter;
    TString UilFilter;
    TString QueryLangFilter;
    TString DocLangFilter;
    TString ItemtypeFilter;
    TString ReportTypeFilter;
    bool MainPageFilter;
    bool InvertFilters;

public:
    bool PassFilter(const TString& sctx) const {
        TSnippetsCtx ctx;
        if (!ParseContext(sctx, ctx)) {
            return false;
        }
        bool pass = PassFilter(ctx);
        return pass != InvertFilters;
    }

private:
    bool PassFilter(const TSnippetsCtx& ctx) const {
        if (ZoneFilter.size() && !ContainsMediawikiZone(ctx, ZoneFilter)) {
            return false;
        }
        if (AttrFilter.size() && !ContainsAttr(ctx, AttrFilter)) {
            return false;
        }
        if (HostFilter.size() && !ContainsHost(ctx, HostFilter)) {
            return false;
        }
        if (UilFilter.size() && !ContainsUil(ctx, UilFilter)) {
            return false;
        }
        if (DocLangFilter.size() && !ContainsDocLang(ctx, DocLangFilter)) {
            return false;
        }
        if (QueryLangFilter.size() && !ContainsQueryLang(ctx, QueryLangFilter)) {
            return false;
        }
        if (ItemtypeFilter.size() && !ContainsItemtype(ctx, ItemtypeFilter)) {
            return false;
        }
        if (ReportTypeFilter.size() && !HasReportType(ctx, ReportTypeFilter)) {
            return false;
        }
        if (MainPageFilter && !IsMainPage(ctx)) {
            return false;
        }
        return true;
    }
};

int main(int argc, char* argv[]) {
    TContextsFilter filter;
    using namespace NLastGetopt;
    TOpts opts = TOpts::Default();
    opts.AddHelpOption();
    opts.AddCharOption('z', "filter contexts with murkup zone")
        .RequiredArgument("ZONE").StoreResult(&filter.ZoneFilter);
    opts.AddCharOption('a', "filter contexts with document attribute")
        .RequiredArgument("ATTR").StoreResult(&filter.AttrFilter);
    opts.AddCharOption('h', "filter contexts with host (accept subdomains if started with '.')")
        .RequiredArgument("HOST").StoreResult(&filter.HostFilter);
    opts.AddCharOption('u', "filter contexts with user language (uil)")
        .RequiredArgument("LANG").StoreResult(&filter.UilFilter);
    opts.AddCharOption('d', "filter contexts with document language ('unknown' for no language)")
        .RequiredArgument("LANG").StoreResult(&filter.DocLangFilter);
    opts.AddCharOption('q', "filter contexts with query language ('unknown' for no language)")
        .RequiredArgument("LANG").StoreResult(&filter.QueryLangFilter);
    opts.AddCharOption('s', "filter contexts with schema.org itemtype (e.g. 'product')")
        .RequiredArgument("ITEMTYPE").StoreResult(&filter.ItemtypeFilter);
    opts.AddCharOption('r', "filter contexts with report type (e.g. 'web3', 'www-touch')")
        .RequiredArgument("REPORTTYPE").StoreResult(&filter.ReportTypeFilter);
    opts.AddCharOption('m', "filter contexts with main page")
        .NoArgument().SetFlag(&filter.MainPageFilter);
    opts.AddCharOption('i', "invert all filters")
        .NoArgument().SetFlag(&filter.InvertFilters);
    opts.SetFreeArgsNum(0);
    TOptsParseResult(&opts, argc, argv);

    TString line;
    while (Cin.ReadLine(line)) {
        if (filter.PassFilter(line)) {
            Cout << line << Endl;
        }
    }

    return 0;
}
