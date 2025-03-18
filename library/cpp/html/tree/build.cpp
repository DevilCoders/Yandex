#include "face.h"

namespace NHtmlTree {
    class TBuildTreeParserResult::TImpl {
    private:
        TMemoryPool* Pool;
        TElementNode* CurrentNode;
        TPoolAlloc<TNode*> NodeAllocator;

    public:
        TImpl(TTree* t)
            : Pool(t->GetStorage())
            , CurrentNode(t->RootNode())
            , NodeAllocator(t->GetStorage())
        {
        }

        void AttachEvent(const THtmlChunk* e) {
            // first any text
            if (e->GetLexType() == HTLEX_TEXT) {
                CurrentNode->Add(new (*Pool) TTextNode(CurrentNode, e));
            }
            // then irregular markup
            else if (e->flags.type == PARSED_MARKUP && e->Tag && e->Tag->is(HT_irreg)) {
                CurrentNode->Add(new (*Pool) TNode(NODE_IRREG, CurrentNode, e));
            }
            // then ignored markup
            else if (e->flags.type == PARSED_MARKUP && e->flags.markup == MARKUP_IGNORED) {
                CurrentNode->Add(new (*Pool) TNode(NODE_MARKUP, CurrentNode, e));
            }
            // then regular markup (HT_br or not -- use elements)
            // empty
            else if (e->GetLexType() == HTLEX_EMPTY_TAG) {
                CurrentNode->Add(new (*Pool) TElementNode(CurrentNode, e, &NodeAllocator));
            }
            // then open
            else if (e->GetLexType() == HTLEX_START_TAG) {
                TElementNode* n = new (*Pool) TElementNode(CurrentNode, e, &NodeAllocator);
                CurrentNode->Add(n);
                CurrentNode = n;
            }
            // then close
            else if (e->GetLexType() == HTLEX_END_TAG) {
                CurrentNode->CloseEvent = e;
                CurrentNode = CurrentNode->Parent;
            }
            // Cdbg << "attached node: " << *e << Endl;
        }
    };

    TBuildTreeParserResult::TBuildTreeParserResult(TTree* doc)
        : TParserResult(doc->GetHtmlStorage())
        , Impl(new TImpl(doc))
    {
    }

    TBuildTreeParserResult::~TBuildTreeParserResult() {
    }

    THtmlChunk* TBuildTreeParserResult::OnHtmlChunk(const THtmlChunk& chunk) {
        THtmlChunk* placedEvent = TParserResult::OnHtmlChunk(chunk);
        Impl->AttachEvent(placedEvent);
        return nullptr;
    }

}
