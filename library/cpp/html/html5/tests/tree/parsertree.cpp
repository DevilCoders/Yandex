#include <library/cpp/html/entity/htmlentity.h>
#include <library/cpp/html/html5/parser.h>
#include <library/cpp/html/html5/foreign.h>

#include <util/generic/buffer.h>
#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/stream/input.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/stream/str.h>

using namespace NHtml5;

namespace {
    class TTreePrinter {
    public:
        TTreePrinter(IOutputStream& out)
            : Output_(out)
        {
        }

        void PrintTree(const TNode* node, const ui32 level = 0) {
            switch (node->Type) {
                case NODE_DOCUMENT: {
                    if (node->Document.HasDoctype) {
                        const TDocument& doc = node->Document;
                        Output_ << "| <!DOCTYPE "
                                << TStringBuf(doc.Name.Data, doc.Name.Length);

                        if (doc.PublicIdentifier.Data || doc.SystemIdentifier.Data) {
                            Output_ << " \""
                                    << ((doc.PublicIdentifier.Data)
                                            ? TString(doc.PublicIdentifier.Data, doc.PublicIdentifier.Length)
                                            : TString())
                                    << "\"";

                            Output_ << " \""
                                    << ((doc.SystemIdentifier.Data)
                                            ? TString(doc.SystemIdentifier.Data, doc.SystemIdentifier.Length)
                                            : TString())
                                    << "\"";
                        }

                        Output_ << ">" << Endl;
                    }

                    PrintChildren(node->Document.Children, level);
                } break;
                case NODE_ELEMENT: {
                    Output_ << "| " << TString(level, ' ');

                    /**
                     * Namespaces.
                     * Unlike in X(HT)ML, namespaces in HTML5 are not denoted by a prefix.  Rather,
                     * anything inside an <svg> tag is in the SVG namespace, anything inside the
                     * <math> tag is in the MathML namespace, and anything else is inside the HTML
                     * namespace.  No other namespaces are supported, so this can be an enum only.
                     */
                    switch (node->Element.TagNamespace) {
                        case NAMESPACE_HTML:
                            Output_ << "<";
                            break;
                        case NAMESPACE_SVG:
                            Output_ << "<svg ";
                            break;
                        case NAMESPACE_MATHML:
                            Output_ << "<math ";
                            break;
                    }

                    TStringPiece origin = GetTagFromOriginalText(node->Element.OriginalTag);

                    if (node->Element.Tag == TAG_UNKNOWN) {
                        switch (node->Element.TagNamespace) {
                            case NAMESPACE_HTML:
                            case NAMESPACE_MATHML:
                                Output_ << to_lower(TString(origin.Data, origin.Length)) << ">" << Endl;
                                break;
                            case NAMESPACE_SVG: {
                                const char* name = NormalizeSvgTagname(origin);

                                if (name == nullptr) {
                                    Output_ << to_lower(TString(origin.Data, origin.Length)) << ">" << Endl;
                                } else {
                                    Output_ << name << ">" << Endl;
                                }
                            } break;
                        }

                    } else {
                        switch (node->Element.TagNamespace) {
                            case NAMESPACE_HTML:
                                Output_ << GetTagName(node->Element.Tag) << ">" << Endl;
                                break;
                            case NAMESPACE_MATHML:
                                Output_ << to_lower(TString(GetTagName(node->Element.Tag))) << ">" << Endl;
                                break;
                            case NAMESPACE_SVG: {
                                const char* name = NormalizeSvgTagname(origin);

                                if (name == nullptr) {
                                    name = GetTagName(node->Element.Tag);
                                }

                                Output_ << name << ">" << Endl;
                            } break;
                        }
                    }

                    const TVectorType<TAttribute>& attrs = node->Element.Attributes;
                    TVector<std::pair<TString, EAttributeNamespace>> data;

                    for (ui32 i = 0; i < attrs.Length; ++i) {
                        const TAttribute* attr = &attrs.Data[i];
                        TString res = TString() + to_lower(TString(attr->OriginalName.Data, attr->OriginalName.Length)) + "=\"";

                        {
                            TString value;
                            if (attr->OriginalName.Data != attr->OriginalValue.Data) {
                                if (attr->OriginalValue.Length > 1 && (attr->OriginalValue.Data[0] == '\'' || attr->OriginalValue.Data[0] == '\"')) {
                                    value += TStringBuf(attr->OriginalValue.Data + 1, attr->OriginalValue.Length - 2);
                                } else {
                                    value += TStringBuf(attr->OriginalValue.Data, attr->OriginalValue.Length);
                                }
                            }
                            res += DecodeAttribute(value);
                        }

                        res += "\"";

                        data.push_back(std::make_pair(res, attr->AttrNamespace));
                    }

                    Sort(data.begin(), data.end(), [](const std::pair<TString, EAttributeNamespace>& a, const std::pair<TString, EAttributeNamespace>& b) { return a.first < b.first; });

                    for (size_t i = 0; i < data.size(); ++i) {
                        Output_ << "| " << TString(level + 2, ' ');

                        switch (data[i].second) {
                            case ATTR_NAMESPACE_NONE:
                                break;
                            case ATTR_NAMESPACE_XLINK:
                                Output_ << "xlink ";
                                break;
                            case ATTR_NAMESPACE_XML:
                                Output_ << "xml ";
                                break;
                            case ATTR_NAMESPACE_XMLNS:
                                Output_ << "xmlns ";
                                break;
                        }

                        Output_ << data[i].first << Endl;
                    }

                    if (node->Element.Tag == TAG_TEMPLATE && node->Element.TagNamespace == NAMESPACE_HTML) {
                        Output_ << "| " << TString(level + 2, ' ') << "content" << Endl;
                        PrintChildren(node->Element.Children, level + 4);
                    } else {
                        PrintChildren(node->Element.Children, level + 2);
                    }
                } break;
                case NODE_TEXT:
                    Output_ << "| " << TString(level, ' ') << "\"" << DecodeText(TString(node->Text.Text.Data, node->Text.Text.Length)) << "\"" << Endl;
                    break;
                case NODE_COMMENT:
                    Output_ << "| " << TString(level, ' ') << "<!-- " << TStringBuf(node->Text.Text.Data, node->Text.Text.Length) << " -->" << Endl;
                    break;
                case NODE_WHITESPACE:
                    Output_ << "| " << TString(level, ' ') << "\"" << TStringBuf(node->Text.Text.Data, node->Text.Text.Length) << "\"" << Endl;
                    break;
            }
        }

    private:
        TString DecodeText(const TString& s) const {
            TBuffer buf(s.size() * 4);
            size_t len = HtEntDecodeToUtf8(CODES_UTF8, s.data(), s.size(), buf.Data(), s.size() * 4);
            return TString(buf.Data(), len);
        }

        TString DecodeAttribute(const TString& value) const {
            TBuffer buf(value.size() * 4);
            size_t len = HtDecodeAttrToUtf8(CODES_UTF8, value.data(), value.size(), buf.Data(), value.size() * 4);
            return TString(buf.Data(), len);
        }

        void PrintChildren(const TVectorType<TNode*>& children, const int level) {
            for (ui32 i = 0; i < children.Length; ++i) {
                PrintTree(children.Data[i], level);
            }
        }

    private:
        IOutputStream& Output_;
    };

}

void PrintParserTree(const TStringBuf& html, IOutputStream& out) {
    TOutput result;
    TParserOptions opts;
    opts.EnableScripting = true;
    opts.CompatiblePlainText = false;
    TParser(opts, html.data(), html.size()).Parse(&result);
    TTreePrinter(out).PrintTree(result.Document);
}
