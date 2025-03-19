#pragma once

#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>
#include <library/cpp/numerator/numerate.h>

struct TDocInfoEx;
class IDocumentDataInserter;

namespace NIndexerCore {

class IDocumentAction {
protected:
    virtual ~IDocumentAction();
public:
    virtual INumeratorHandler* OnDocProcessingStart(const TDocInfoEx* docInfo, bool isFullProcessing);
    virtual void OnDocProcessingStop(const IParsedDocProperties* pars, const TDocInfoEx* docInfo, IDocumentDataInserter*, bool isFullProcessing);
    virtual void Term();
};

class TDocumentActions : public IDocumentAction, private TNonCopyable {
private:
    typedef TVector<IDocumentAction*> TActions;
    TActions Actions;
    TNumeratorHandlers NumeratorHandlers;
public:
    TDocumentActions();
    ~TDocumentActions() override;
    void AddAction(IDocumentAction* action);

    INumeratorHandler* OnDocProcessingStart(const TDocInfoEx* docInfo, bool isFullProcessing) override;
    void OnDocProcessingStop(const IParsedDocProperties* pars, const TDocInfoEx* docInfo, IDocumentDataInserter* inserter, bool isFullProcessing) override;
    void Term() override;
};

}
