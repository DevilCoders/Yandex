#include "breadcrumbs.h"
#include "tags.h"
#include <library/cpp/html/face/propface.h>
#include <library/cpp/html/face/event.h>
#include <library/cpp/html/entity/htmlentity.h>

namespace NForumsImpl {

bool TBreadcrumbs::CheckText(wchar16* text, size_t len)
{
    bool processed = true;
    if (BreadcrumbDetectorState == InBreadcrumb && BreadcrumbTrackerDepth)
        BreadcrumbText.append(text, len);
    else if (len == 1 && IsArrowCharacter(text[0]) ||
        len == 2 && (text[0] == '-' && text[1] == '>' || text[0] == ':' && text[1] == ':'))
    {
        // if this is a trail separated with ::, keep looking:
        // sometimes it is just a link set from a generic heading
        int curQuality = (text[0] == ':') ? 1 : 2;
        if (BreadcrumbDetectorState == LinkJustClosed) {
            if (BreadcrumbQuality < curQuality) {
                BreadcrumbQuality = curQuality;
                BreadcrumbDetectorState = PossibleBreadcrumb;
            } else
                BreadcrumbDetectorState = Normal;
        } else
            BreadcrumbDetectorState = InBreadcrumb;
        BreadcrumbText.clear();
    } else
        processed = false;
    return processed;
}

void TBreadcrumbs::OnSpecificBreadcrumbTransition(void* param, bool entered, const TNumerStat&, TPostDescriptor*& lastPost)
{
    TBreadcrumbs* this_ = (TBreadcrumbs*)param;
    if (entered) {
        if (lastPost)
            return;
        this_->CurText.clear();
        this_->InAnyBreadcrumb = true;
    } else if (!lastPost && !this_->CurText.empty()) {
        TWtringBuf w(this_->StringsPool.append(this_->CurText.data(), this_->CurText.size()), this_->CurText.size());
        this_->BreadcrumbTrail.push_back(w);
        this_->InAnyBreadcrumb = false;
        this_->CurText.clear();
    }
}

void TBreadcrumbs::CheckBreadcrumbOpenTag(const THtmlChunk* chunk, int depth, const IParsedDocProperties* ps)
{
    if (!BreadcrumbTrackerDepth && HrefTag.Match(chunk)) {
        BreadcrumbTrackerDepth = depth;
        if (BreadcrumbDetectorState == PossibleBreadcrumb) {
            BreadcrumbDetectorState = InBreadcrumb;
            BreadcrumbText.clear();
            if (!PrevLinkText)
                return;
            TTempArray<wchar16> unicodeTempBuf(PrevLinkTextSize * 2);
            size_t bufLen = HtEntDecodeToChar(ps->GetCharset(), PrevLinkText, PrevLinkTextSize, unicodeTempBuf.Data());
            PrevLinkText = nullptr;
            PrevLinkTextSize = 0;
            if (!bufLen)
                return;
            wchar16* ptr = unicodeTempBuf.Data();
            wchar16* end = ptr + bufLen;
            while (ptr != end && IsSpace(*ptr))
                ptr++;
            while (ptr != end && IsSpace(end[-1]))
                end--;
            if (ptr != end) {
                BreadcrumbTrail.clear();
                BreadcrumbTrail.push_back(TWtringBuf(StringsPool.append(ptr, end - ptr), end - ptr));
            }
        } else if (BreadcrumbDetectorState != InBreadcrumb)
            BreadcrumbDetectorState = Normal;
    }
}

void TBreadcrumbs::CheckBreadcrumbText(const char* text, unsigned len)
{
    if (BreadcrumbDetectorState == Normal) {
        if (BreadcrumbTrackerDepth) {
            PrevLinkText = text;
            PrevLinkTextSize = len;
        } else {
            PrevLinkText = nullptr;
            PrevLinkTextSize = 0;
        }
    }
}

void TBreadcrumbs::CheckBreadcrumbCloseTag(const THtmlChunk* chunk, int depth)
{
    if (BreadcrumbTrackerDepth == depth) {
        BreadcrumbTrackerDepth = 0;
        if (BreadcrumbDetectorState == Normal && PrevLinkText)
            BreadcrumbDetectorState = LinkJustClosed;
        else if (BreadcrumbDetectorState == InBreadcrumb) {
            bool onlySpaces = true;
            for (TUtf16String::const_iterator it = BreadcrumbText.begin(); it != BreadcrumbText.end(); ++it)
                if (!IsSpace(*it)) {
                    onlySpaces = false;
                    break;
                }
            if (onlySpaces) {
                BreadcrumbDetectorState = PossibleBreadcrumb;
            } else {
                BreadcrumbTrail.push_back(TWtringBuf(StringsPool.append(BreadcrumbText.data(), BreadcrumbText.size()), BreadcrumbText.size()));
                BreadcrumbDetectorState = InBetweenBreadcrumb;
            }
            BreadcrumbText.clear();
        }
    }
    if (chunk->Tag && *chunk->Tag == HT_TR)
        BreadcrumbDetectorState = Normal;
}

} // namespace NForumsImpl
