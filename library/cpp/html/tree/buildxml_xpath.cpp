#include "buildxml_xpath.h"

#include <libxml/tree.h>

#include <library/cpp/charset/recyr.hh>
#include <util/string/cast.h>

#include <library/cpp/html/entity/htmlentity.h>
#include <library/cpp/html/face/propface.h>
#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/hash.h>

namespace NHtmlTree {
    using namespace NXml;

    class TXmlDomBuilder::TImpl {
    public:
        TImpl(const IParsedDocProperties& docProperties)
            : DocProperties(docProperties)
            , Tree(new TXmlTree())
            , Doc(Tree->GetXmlDoc())
            , CurrentNode(xmlNewNode(nullptr, (xmlChar*)"html"))
        {
            Doc.SetRoot(CurrentNode);
        }

        TSimpleSharedPtr<TXmlTree> GetTree() const {
            return Tree;
        }

        void OnMoveInput(const THtmlChunk& chunk, const TZoneEntry*, const TNumerStat& numerStat) {
            Y_ASSERT(CurrentNode != nullptr);

            if (chunk.GetLexType() == HTLEX_TEXT) {
                xmlNode* node = nullptr;
                ECharset sourceCharset = DocProperties.GetCharset();
                if (sourceCharset != CODES_UTF8) {
                    TString recodedText;
                    Recode(sourceCharset, CODES_UTF8, TStringBuf(chunk.text, chunk.leng), recodedText);
                    node = xmlNewTextLen((xmlChar*)recodedText.data(), recodedText.size());
                } else {
                    node = xmlNewTextLen((xmlChar*)chunk.text, chunk.leng);
                }

                xmlAddChild(CurrentNode, node);
            } else if (chunk.GetLexType() == HTLEX_EMPTY_TAG) {
                xmlNode* node = xmlNewChild(CurrentNode, nullptr, (xmlChar*)chunk.Tag->lowerName, nullptr);

                AttachStartPosition(CurrentNode, GetPosting(numerStat));
                AttachEndPosition(CurrentNode, GetPosting(numerStat)); //< not required, but for consistency
                AttachAttrs(node, chunk);
            } else if (chunk.GetLexType() == HTLEX_START_TAG) {
                if (!IsTagFiltered(chunk.Tag->lowerName)) {
                    if (!chunk.Tag->is(HT_HTML)) {
                        // don't create a second <html>
                        CurrentNode = xmlNewChild(CurrentNode, /* ns */ nullptr, (xmlChar*)chunk.Tag->lowerName, /* content */ nullptr);
                    }

                    AttachStartPosition(CurrentNode, GetPosting(numerStat));
                    AttachEndPosition(CurrentNode, GetPosting(numerStat)); //< not required, but for consistency
                    AttachAttrs(CurrentNode, chunk);
                }
            } else if (chunk.GetLexType() == HTLEX_END_TAG) {
                if (!IsTagFiltered(chunk.Tag->lowerName)) {
                    if (!chunk.Tag->is(HT_HTML)) {
                        if (strcmp(chunk.Tag->lowerName, (const char*)CurrentNode->name) == 0) {
                            AttachEndPosition(CurrentNode, GetPosting(numerStat));
                            CurrentNode = CurrentNode->parent;
                        } else {
                            // Cerr << "Trying to match non-matching tags (" << chunk.Tag->lowerName << " and " << (const char*) CurrentNode->name << "). Skip" << Endl;
                        }
                    }
                }
            }
        }

    private:
        TPosting GetPosting(const TNumerStat& numerStat) const {
            return (TPosting)numerStat.TokenPos.SuperLong();
        }

        void EnsureNodeData(xmlNode* node) {
            if (node->_private == nullptr) {
                TNodeData* nodeData = Tree->AllocateNodeData();
                node->_private = (void*)nodeData;
            }
        }

        void AttachStartPosition(xmlNode* node, TPosting start) {
            EnsureNodeData(node);

            TNodeData& nodeData = *reinterpret_cast<TNodeData*>(node->_private);
            nodeData.StartPosition = start;
        }

        void AttachEndPosition(xmlNode* node, TPosting end) {
            EnsureNodeData(node);

            TNodeData& nodeData = *reinterpret_cast<TNodeData*>(node->_private);
            nodeData.EndPosition = end;
        }

        void AttachAttrs(xmlNode* node, const THtmlChunk& event) {
            for (size_t i = 0; i < event.AttrCount; ++i) {
                TString name = TString(
                    event.text + event.Attrs[i].Name.Start,
                    event.Attrs[i].Name.Leng);
                name.to_lower();

                TString val = TString(
                    event.text + event.Attrs[i].Value.Start,
                    event.Attrs[i].Value.Leng);

                xmlAttr* attr = xmlNewProp(node, (xmlChar*)name.data(), (xmlChar*)val.data());
                attr->_private = (void*)&event;
            }
        }

        bool IsTagFiltered(const char* tagLowerName) {
            return !strcmp("noscript", tagLowerName) ||
                   !strcmp("noindex", tagLowerName);
        }

    private:
        const IParsedDocProperties& DocProperties;
        TSimpleSharedPtr<TXmlTree> Tree;

        TXmlDoc& Doc;
        xmlNode* CurrentNode;
    };

    TXmlDomBuilder::TXmlDomBuilder(const IParsedDocProperties& docProperties)
        : Impl(new TImpl(docProperties))
    {
    }

    TXmlDomBuilder::~TXmlDomBuilder() {
    }

    void TXmlDomBuilder::OnMoveInput(const THtmlChunk& chunk, const TZoneEntry* zoneEntry, const TNumerStat& numerStat) {
        Impl->OnMoveInput(chunk, zoneEntry, numerStat);
    }

    TSimpleSharedPtr<TXmlTree> TXmlDomBuilder::GetTree() const {
        return Impl->GetTree();
    }

}
