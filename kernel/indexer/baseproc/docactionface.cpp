#include "docactionface.h"

namespace NIndexerCore {

IDocumentAction::~IDocumentAction() {
}

INumeratorHandler* IDocumentAction::OnDocProcessingStart(const TDocInfoEx* /*docInfo*/, bool /*isFullProcessing*/) {
    return nullptr;
}

void IDocumentAction::OnDocProcessingStop(const IParsedDocProperties* /*pars*/, const TDocInfoEx* /*docInfo*/, IDocumentDataInserter*, bool /*isFullProcessing*/) {
}

void IDocumentAction::Term() {
}

TDocumentActions::TDocumentActions() {
}

TDocumentActions::~TDocumentActions() {
}

void TDocumentActions::AddAction(IDocumentAction* action) {
    Actions.push_back(action);
}

INumeratorHandler* TDocumentActions::OnDocProcessingStart(const TDocInfoEx* docInfo, bool isFullProcessing) {
    for (TActions::iterator it = Actions.begin(); it != Actions.end(); ++it) {
        IDocumentAction* pp = *it;
        INumeratorHandler* nh = pp->OnDocProcessingStart(docInfo, isFullProcessing);
        if (nh)
            NumeratorHandlers.AddHandler(nh);
    }
    return &NumeratorHandlers;
}

void TDocumentActions::OnDocProcessingStop(const IParsedDocProperties* pars, const TDocInfoEx* docInfo, IDocumentDataInserter* inserter, bool isFullProcessing) {
    for (TActions::iterator it = Actions.begin(); it != Actions.end(); ++it) {
        IDocumentAction* pp = *it;
        pp->OnDocProcessingStop(pars, docInfo, inserter, isFullProcessing);
    }
    NumeratorHandlers.Clear();
}

void TDocumentActions::Term() {
    for (TActions::iterator it = Actions.begin(); it != Actions.end(); ++it) {
        IDocumentAction* pp = *it;
        pp->Term();
    }
    Actions.clear();
}

}
