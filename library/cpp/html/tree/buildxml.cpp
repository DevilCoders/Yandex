#include "buildxml.h"
#include <libxml/tree.h>

namespace NHtmlTree {
    using namespace NXml;

    class TBuildXmlTreeParserResult::TImpl {
    private:
        // TMemoryPool* Pool;
        NXml::TDocument* Doc;
        TNode CurrentNode;
        // TElementNode* CurrentNode;
        // TPoolAlloc<TNode*> NodeAllocator;
    public:
        TImpl(NXml::TDocument* doc)
            : Doc(doc)
            , CurrentNode(Doc->Root())
        {
        }

        void AttachEvent(const THtmlChunk* e) {
            void* priv = (void*)e;
            Y_ASSERT(!CurrentNode.IsNull() && "node is null");
            // first any text
            if (e->GetLexType() == HTLEX_TEXT) {
                TNode node = CurrentNode.AddText(TString(e->text, e->leng));
                node.SetPrivate(priv);
            }
            // then irregular markup
            else if (e->flags.type == PARSED_MARKUP && e->Tag && e->Tag->is(HT_HTML)) {
                // skip root
            } else if (e->flags.type == PARSED_MARKUP && e->Tag && e->Tag->is(HT_irreg)) {
                // skip irreg
            }
            // then ignored markup
            else if (e->flags.type == PARSED_MARKUP && e->flags.markup == MARKUP_IGNORED) {
                // ignore
            }
            // then regular markup (HT_br or not -- use elements)
            // empty
            else if (e->GetLexType() == HTLEX_EMPTY_TAG) {
                TNode node = CurrentNode.AddChild(e->Tag->lowerName);
                node.SetPrivate(priv);
                AttachAttrs(node, e);
            }
            // then open
            else if (e->GetLexType() == HTLEX_START_TAG) {
                CurrentNode = CurrentNode.AddChild(e->Tag->lowerName);
                CurrentNode.SetPrivate(priv);
                AttachAttrs(CurrentNode, e);
            }
            // then close
            else if (e->GetLexType() == HTLEX_END_TAG) {
                CurrentNode = CurrentNode.Parent();
            }
        }

    private:
        void AttachAttrs(TNode& node, const THtmlChunk* event) {
            for (size_t i = 0; i < event->AttrCount; ++i) {
                const TString name = TString(event->text + event->Attrs[i].Name.Start, event->Attrs[i].Name.Leng);
                const TString val = TString(event->text + event->Attrs[i].Value.Start, event->Attrs[i].Value.Leng);
                node.SetAttr(name, val);
            }
        }
    };

    TBuildXmlTreeParserResult::TBuildXmlTreeParserResult(NXml::TDocument* doc)
        : Impl(new TImpl(doc))
    {
    }

    TBuildXmlTreeParserResult::~TBuildXmlTreeParserResult() {
    }

    THtmlChunk* TBuildXmlTreeParserResult::OnHtmlChunk(const THtmlChunk& chunk) {
        Impl->AttachEvent(&chunk);
        return nullptr;
    }

}
