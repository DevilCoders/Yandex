#include "directtextaction.h"
#include <library/cpp/html/face/parsface.h>
#include <kernel/indexer/dtcreator/dtcreator.h>
#include <kernel/indexer/dtcreator/dthandler.h>
#include <kernel/indexer/face/inserter.h>
#include <kernel/indexer/face/docinfo.h>
#include <kernel/keyinv/invkeypos/keynames.h>

#include <yweb/robot/dbscheeme/urlflags.h>

namespace NIndexerCore {

TDirectTextAction::TDirectTextAction(TDirectTextCreator& dtc)
    : DTCreator(dtc)
{
}

TDirectTextAction::~TDirectTextAction() {
}

INumeratorHandler* TDirectTextAction::OnDocProcessingStart(const TDocInfoEx* /*docInfo*/, bool /*isFullProcessing*/) {
    TextHandler.Reset(new TDirectTextHandler(DTCreator));
    return TextHandler.Get();
}

void TDirectTextAction::OnDocProcessingStop(const IParsedDocProperties* pars, const TDocInfoEx* docInfo, IDocumentDataInserter* inserter, bool isFullProcessing) {
    TextHandler.Destroy();
    if (isFullProcessing)
        InsertAttributes(pars, docInfo, inserter);
}

void TDirectTextAction::InsertAttributes(const IParsedDocProperties* pars, const TDocInfoEx* docInfo, IDocumentDataInserter* inserter) {
        const char *robots = nullptr;
        if (!pars->GetProperty(PP_ROBOTS, &robots)) {
            if (strlen(robots) == 5) {
                if (robots[2] == '0' || (docInfo->UrlFlags & UrlFlags::NOARCHIVE)) {
                    inserter->StoreTextArchiveDocAttr("noarchive", "yes");
                    inserter->StoreFullArchiveDocAttr("noarchive", "yes");
                }
                if (robots[3] == '0') {
                    inserter->StoreTextArchiveDocAttr("noodp", "yes");
                    inserter->StoreFullArchiveDocAttr("noodp", "yes");
                }
                if (robots[4] == '0') {
                    inserter->StoreTextArchiveDocAttr("noyaca", "yes");
                    inserter->StoreFullArchiveDocAttr("noyaca", "yes");
                }
            }

        }

        if (docInfo->DocHeader->Language2) {
            char buf[16];
            snprintf(buf, 16, "%u", docInfo->DocHeader->Language2);
            inserter->StoreTextArchiveDocAttr("Language2", buf);
        }

        if (docInfo->DocHeader->IndexDate != 0)
            inserter->StoreDateTimeAttr(YDX_INDEX_DATE, docInfo->DocHeader->IndexDate);
}

}
