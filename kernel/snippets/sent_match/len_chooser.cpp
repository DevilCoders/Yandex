#include "len_chooser.h"

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/qtree/query.h>

#include <util/generic/utility.h>

#include <cmath>

namespace NSnippets {

TLengthChooser::TLengthChooser(const TConfig& cfg, const TQueryy& query)
    : Cfg(cfg)
    , Query(query)
    , CutMethod(Cfg.GetSnipCutMethod())
{
}

float TLengthChooser::GetMaxSnipLen(int frags) const {
    const float requested = Cfg.GetRequestedMaxSnipLen();
    if (requested > 0) {
        return requested;
    }
    if (Cfg.ImgSearch()) {
        Y_ASSERT(CutMethod == TCM_SYMBOL);
        return Min(120, 70 + 10 * Query.PosCount());
    }
    if (Cfg.VideoLength()) {
        Y_ASSERT(CutMethod == TCM_SYMBOL);
        return Min(75, 35 + 8 * Query.PosCount());
    }

    if (Cfg.GetBigDescrForMainBna() && Cfg.GetMainBna()) {
        if (Cfg.GetMaxBnaSnipRowCount() != 0) {
            return GetNDesktopRowsLen(Cfg.GetMaxBnaSnipRowCount());
        }
        if (Cfg.GetMaxBnaRealSnipRowCount() != 0) {
            return Cfg.GetMaxBnaRealSnipRowCount();
        }
        if (frags == 1) {
            return GetNDesktopRowsLen(4);
        }
    }
    if (Cfg.BNASnippets()) {
        return GetNDesktopRowsLen(2);
    }

    if (frags == 3 || frags == 4) {
        return GetNDesktopRowsLen(frags);
    } else if (Query.UserPosCount() < 4) {
        return GetNDesktopRowsLen(Cfg.GetMaxSnipRowCount());
    } else {
        return SymbolsToWholeRows(Cfg.GetMaxSnipRowCount() * 100 + 50);
    }
}

float TLengthChooser::GetRowLen() const {
    return SymbolsToWholeRows(95);
}

float TLengthChooser::GetMaxSpecSnipLen() const {
    if (Cfg.GetBigDescrForMainBna() && Cfg.GetMainBna()) {
        if (Cfg.GetMaxBnaSnipRowCount() != 0) {
            return GetNDesktopRowsLen(Cfg.GetMaxBnaSnipRowCount());
        }
        if (Cfg.GetMaxBnaRealSnipRowCount() != 0) {
            return Cfg.GetMaxBnaRealSnipRowCount();
        }
    }
    const float requested = Cfg.GetRequestedMaxSnipLen();
    if (requested > 0) {
        return requested;
    }
    return SymbolsToWholeRows(250);
}

float TLengthChooser::GetMaxTextSpecSnipLen() const {
    if (!Cfg.ExpFlagOff("short_spec")) {
        return GetMaxSnipLen();
    }
    return GetMaxSpecSnipLen();
}

float TLengthChooser::GetNDesktopRowsLen(int rows) const {
    const float requested = Cfg.GetRequestedMaxSnipLen();
    if (requested > 0) {
        return requested;
    }
    return SymbolsToWholeRows(95 * rows);
}

float TLengthChooser::GetMaxByLinkSnipLen() const {
    const float requested = Cfg.GetRequestedMaxSnipLen();
    float byLinkLen = 0.0f;
    if (CutMethod == TCM_PIXEL) {
        byLinkLen = Cfg.GetByLinkLen();
    } else {
        byLinkLen = 95;
    }
    if (requested > 0 && requested < byLinkLen) {
        return requested;
    }
    return byLinkLen;
}

void TLengthChooser::SetShortenRowScale() {
    ShortenRowScale = true;
}

float TLengthChooser::SymbolsToWholeRows(int symbols) const {
    if (CutMethod != TCM_PIXEL) {
        return symbols;
    }
    const float rowScale = ShortenRowScale ? 1.0f : Cfg.GetRowScale();
    const float symbolsScale = symbols / 95.0f;
    const float fontSizeScale = Cfg.GetSnipFontSize() / 13.0f;
    const float pixelsInLineScale = Cfg.GetYandexWidth() / static_cast<float>(607 - 8);
    return roundf(rowScale * symbolsScale * fontSizeScale / pixelsInLineScale);
}

}
