#include "semantic.h"

#include <kernel/snippets/schemaorg/proto/schemaorg.pb.h>
#include <kernel/snippets/schemaorg/schemaorg_serializer.h>

#include <kernel/snippets/util/xml.h>

#include <kernel/info_request/inforequestformatter.h>

#include <util/charset/wide.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

namespace NSnippets {

static void PrintSchemaOrgField(IOutputStream& output, const TString& name, const TString& value, const TString& indent) {
    output.Write(indent);
    output.Write(name);
    output.Write(" = ");
    output.Write(EncodeTextForXml10(value, true));
    output.Write("<br>\n");
}

static void PrintSchemaOrgTree(IOutputStream& output, const NSchemaOrg::TTreeNode& node, int level) {
    TString indent = WideToUTF8(TUtf16String(level * 6, wchar16(160)));
    for (const auto& itemType : node.GetItemtypes()) {
        PrintSchemaOrgField(output, "itemtype", itemType, indent);
    }
    for (const auto& itemProp : node.GetItemprops()) {
        PrintSchemaOrgField(output, "itemprop", itemProp, indent);
    }
    if (node.HasText()) {
        PrintSchemaOrgField(output, "text", node.GetText(), indent);
    }
    if (node.HasHref()) {
        PrintSchemaOrgField(output, "href", node.GetHref(), indent);
    }
    if (node.HasDatetime()) {
        PrintSchemaOrgField(output, "datetime", node.GetDatetime(), indent);
    }
    for (size_t i = 0; i < node.IdSize(); ++i) {
        PrintSchemaOrgField(output, "id", node.GetId(i), indent);
    }
    for (size_t i = 0; i < node.ItemrefSize(); ++i) {
        PrintSchemaOrgField(output, "itemref", node.GetItemref(i), indent);
    }
    if (node.HasSentBegin()) {
        PrintSchemaOrgField(output, "sentbegin", ToString(node.GetSentBegin()), indent);
    }
    if (node.HasSentCount()) {
        PrintSchemaOrgField(output, "sentcount", ToString(node.GetSentCount()), indent);
    }
    for (size_t i = 0; i < node.NodeSize(); ++i) {
        output.Write(indent);
        output.Write("<b>node</b><br>\n");
        PrintSchemaOrgTree(output, node.GetNode(i), level + 1);
    }
}

static TString PrintSchemaOrgMarkup(const TDocInfos& docInfos) {
    TDocInfos::const_iterator schemaOrgAttr = docInfos.find("SchemaOrg");
    if (schemaOrgAttr == docInfos.end()) {
        return "SchemaOrg attribute not found";
    }
    NSchemaOrg::TTreeNode root;
    if (!NSchemaOrg::DeserializeFromBase64(schemaOrgAttr->second, root)) {
        return "Deserialization error";
    }
    TStringStream output;
    PrintSchemaOrgTree(output, root, 0);
    return output.Str();
}

bool HasSemanticMarkup(const TDocInfos& docInfos) {
    return docInfos.contains("SchemaOrg");
}

void PrintSemanticMarkup(const TDocInfos& docInfos, IInfoDataTable* result) {
    TVector<TInfoDataCell> cells;

    cells.push_back(TInfoDataCell("Schema.org"));
    result->AddRow(cells, RT_HEADER);

    cells.clear();
    cells.push_back(TInfoDataCell(PrintSchemaOrgMarkup(docInfos)));
    result->AddRow(cells, RT_DATA);
}

}
