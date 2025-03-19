#pragma once

#include "docactionface.h"

#include <util/generic/ptr.h>

namespace NIndexerCore {

class TDirectTextCreator;
class TDirectTextHandler;

class TDirectTextAction : public IDocumentAction, private TNonCopyable {
private:
    TDirectTextCreator& DTCreator;
    THolder<TDirectTextHandler> TextHandler;
public:
    TDirectTextAction(TDirectTextCreator& dtc);
    ~TDirectTextAction() override;

    INumeratorHandler* OnDocProcessingStart(const TDocInfoEx* docInfo, bool isFullProcessing) override;
    void OnDocProcessingStop(const IParsedDocProperties* pars, const TDocInfoEx* docInfo, IDocumentDataInserter* inserter, bool isFullProcessing) override;

    static void InsertAttributes(const IParsedDocProperties* pars, const TDocInfoEx* docInfo, IDocumentDataInserter* inserter);
};

}
