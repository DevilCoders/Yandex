#pragma once

#include "pdstorageconf.h"

#include <library/cpp/html/pdoc/pds.h>
#include <library/cpp/mime/types/mime.h>
#include <util/generic/ptr.h>

class INumeratorHandler;
class IParsedDocProperties;
struct TDocInfoEx;
class TRecognizerShell;

namespace NIndexerCore {

class TParsedDocStorage : protected TSimpleParsedDocStorage {
public:
    TParsedDocStorage(const TParsedDocStorageConfig& cfg);
    virtual ~TParsedDocStorage();

private:
    // the following virtual stuff has been extracted to derived classes to avoid unuseful dependences here.
    virtual void DoParseUnknownFormat(IParsedDocProperties* /*docProps*/, const TDocInfoEx* /*docInfo*/) {
    }
    virtual void DoNumerateUnknownFormat(MimeTypes /*mimeType*/, INumeratorHandler& /*handler*/, IParsedDocProperties* /*docProps*/) {
    }
    virtual void OnAfterParseDoc(IParsedDocProperties* /*docProps*/, const TDocInfoEx* /*docInfo*/) {
    }
public:
    void SetConf(const TString& name);

    void ParseDoc(IParsedDocProperties* docProps, const TDocInfoEx* docInfo);
    void RecognizeDoc(IParsedDocProperties* docProps, const TDocInfoEx* docInfo);
    void NumerateDoc(INumeratorHandler& handler, IParsedDocProperties* docProps, const TDocInfoEx* docInfo);
protected:
    void DoParseHtml(const char* doc, size_t sz, IParsedDocProperties* docProps);
    void DoNumerateHtml(INumeratorHandler& handler, IParsedDocProperties* docProps);
    const THtConfigurator* GetHtConf() const;

protected:
    THolder<TRecognizerShell> Recognizer;
private:
    class THtConfs;
    THolder<THtConfs> HtConfs;
};

}
