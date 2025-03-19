#pragma once
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/charset/wide.h>
#include <util/memory/segmented_string_pool.h>

struct TNumerStat;
struct THtmlChunk;
class IParsedDocProperties;

namespace NForumsImpl {

struct TPostDescriptor;

class TBreadcrumbs
{
private:
    TUtf16String& CurText;
    TUtf16String BreadcrumbText;
    TVector<TWtringBuf> BreadcrumbTrail;
    bool InAnyBreadcrumb;
    const char* PrevLinkText;
    unsigned PrevLinkTextSize;
    // There are two independent mechanisms for breadcrums detection.
    // The first is controlled by InBreadcrumbSpecific/InAnyBreadcrumb above,
    // uses HTML markup for detection and handles specific cases like
    // <li class="navbit"><a href=...>text</a></li>, where
    // CSS has .navbit {background-image:url("ArrowPicture.png")}.
    // The second is controlled by BreadcrumbDetectorState/BreadcrumbTracker
    // below, detects sequences <a href=...>text1</a> >> <a href=...>text2</a> >> ...
    enum {
        Normal,
        LinkJustClosed,
        PossibleBreadcrumb,
        InBreadcrumb,
        InBetweenBreadcrumb,
    } BreadcrumbDetectorState;
    int BreadcrumbTrackerDepth;
    int BreadcrumbQuality;
    segmented_pool<wchar16>& StringsPool;
public:
    TBreadcrumbs(TUtf16String& curText, segmented_pool<wchar16>& stringsPool)
        : CurText(curText)
        , StringsPool(stringsPool)
    {
    }
    static void OnSpecificBreadcrumbTransition(void* param, bool entered, const TNumerStat&, TPostDescriptor*& lastPost);
    void CheckBreadcrumbOpenTag(const THtmlChunk* chunk, int depth, const IParsedDocProperties* ps);
    void CheckBreadcrumbText(const char* text, unsigned len);
    void CheckBreadcrumbCloseTag(const THtmlChunk* chunk, int depth);
    bool NeedCheckText() const
    {
        return BreadcrumbDetectorState != Normal;
    }
    bool CheckText(wchar16* text, size_t len);
    bool NeedText() const
    {
        return InAnyBreadcrumb;
    }
    bool Empty() const
    {
        return BreadcrumbTrail.empty();
    }
    void Clear()
    {
        InAnyBreadcrumb = false;
        BreadcrumbTrackerDepth = 0;
        BreadcrumbTrail.clear();
        PrevLinkText = nullptr;
        PrevLinkTextSize = 0;
        BreadcrumbDetectorState = Normal;
        BreadcrumbQuality = 0;
    }
    void Stop()
    {
        BreadcrumbDetectorState = Normal;
    }
    const TWtringBuf& GetLast() const
    {
        Y_ASSERT(!Empty());
        return BreadcrumbTrail.back();
    }
    void DeleteLast()
    {
        Y_ASSERT(!Empty());
        BreadcrumbTrail.pop_back();
    }
    void PackToString(TString& breadcrumb)
    {
        for (TVector<TWtringBuf>::const_iterator it = BreadcrumbTrail.begin(); it != BreadcrumbTrail.end(); ++it) {
            if (!breadcrumb.empty())
                breadcrumb += '\t';
            breadcrumb += WideToUTF8(*it);
        }
    }
};

} // namespace NForumsImpl
