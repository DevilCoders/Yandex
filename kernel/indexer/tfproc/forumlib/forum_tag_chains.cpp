#include "forum_tag_chains.h"
#include <util/generic/strbuf.h>

bool TTagDescriptor::Match(const THtmlChunk* chunk) const
{
    if (Tag != HT_any && *chunk->Tag != Tag)
        return false;
    if (!AttrName)
        return true;
    for (size_t j = 0; j < chunk->AttrCount; j++) {
        size_t valLen = chunk->Attrs[j].Value.Leng;
        if (chunk->Attrs[j].Name.Leng == AttrNameLen &&
            !memcmp(chunk->text + chunk->Attrs[j].Name.Start, AttrName, AttrNameLen) &&
            (AttrValueMatch == NotPrefix || valLen >= AttrValueLen) &&
            (AttrValueMatch >= Substring || !memcmp(chunk->text + chunk->Attrs[j].Value.Start, AttrValue, AttrValueLen)))
        {
            switch (AttrValueMatch) {
            case ExactMatch:
                if (valLen == AttrValueLen)
                    return true;
                break;
            case PrefixWithoutDigits:
                if (valLen > AttrValueLen) {
                    const char* p = chunk->text + chunk->Attrs[j].Value.Start;
                    const char* q = p + valLen;
                    p += AttrValueLen;
                    for (; p < q; p++)
                        if (*p < '0' || *p > '9')
                            break;
                    if (p == q)
                        return true;
                }
                break;
            case Prefix:
                return true;
            case WordPrefix:
                if (valLen == AttrValueLen || chunk->text[chunk->Attrs[j].Value.Start + AttrValueLen] == ' ')
                    return true;
                break;
            case Substring:
                if (TStringBuf(chunk->text + chunk->Attrs[j].Value.Start, valLen).find(TStringBuf(AttrValue, AttrValueLen)) != TStringBuf::npos)
                    return true;
                break;
            case NotPrefix:
                if (valLen < AttrValueLen)
                    return true;
                if (memcmp(chunk->text + chunk->Attrs[j].Value.Start, AttrValue, AttrValueLen))
                    return true;
                break;
            }
        }
    }
    return false;
}

TTagChainTracker::EStateChange TTagChainTracker::OnOpenTag(const THtmlChunk* chunk, int depth, HT_TAG& watchOpenTag, HT_TAG& watchCloseTag)
{
    Y_ASSERT(Found.empty() || !(Found.back().Tag->Flags & TTagDescriptor::NoDescent));
    Y_ASSERT(Associated);
    Y_ASSERT(NextTag != Associated->Chain + Associated->NumTags);
    if (!NextTag->Match(chunk)) {
        watchOpenTag = NextTag->Tag;
        watchCloseTag = Found.empty() ? HT_TagCount : Found.back().Tag->Tag;
        return NotChanged;
    }
    if (!WasFull && Found.size() == 1)
        Found.back().Depth = depth - 1; // select the most-nested tag
    TLocation location(depth, NextTag++);
    Found.push_back(location);
    watchCloseTag = location.Tag->Tag;
    if (location.Tag->Flags & TTagDescriptor::NoDescent) {
        watchOpenTag = HT_TagCount;
        if (Full) {
            Full = false;
            return AreaLeft;
        } else
            return NotChanged;
    }
    const TTagDescriptor* t = NextTag;
    const TTagDescriptor* end = Associated->Chain + Associated->NumTags;
    watchOpenTag = (t == end) ? HT_TagCount : t->Tag;
    while (t != end && t->Flags & TTagDescriptor::NoDescent)
        t++;
    if (t == end) {
        Full = true;
        WasFull = true;
        return AreaEntered;
    }
    return NotChanged;
}

TTagChainTracker::EStateChange TTagChainTracker::OnCloseTag(const THtmlChunk* /*chunk*/, int depth, HT_TAG& watchOpenTag, HT_TAG& watchCloseTag)
{
    const TTagDescriptor* prevTag = nullptr;
    static const TTagDescriptor fakeTag =
    { HT_any, 0, nullptr, 0, nullptr, 0, TTagDescriptor::ExactMatch };

    EStateChange result = NotChanged;
    while (!Found.empty() && Found.back().Depth >= depth) {
        prevTag = Found.back().Tag;
        Found.pop_back();
        if (prevTag == &fakeTag) {
            while (--NextTag > Associated->Chain)
                if (NextTag->Flags & TTagDescriptor::WaitForParentClose)
                    break;
        }
    }
    if (!prevTag) {
        watchOpenTag = (NextTag == Associated->Chain + Associated->NumTags) ? HT_TagCount : NextTag->Tag;
        if (Found.empty()) {
            watchCloseTag = HT_TagCount;
        } else {
            watchCloseTag = Found.back().Tag->Tag;
            if (Found.back().Tag->Flags & TTagDescriptor::NoDescent)
                watchOpenTag = HT_TagCount;
        }
        return NotChanged;
    }
    if (prevTag->Flags == 0) {
        if (prevTag != &fakeTag)
            NextTag = prevTag;
        if (Full)
            result = AreaLeft;
        Full = false;
        watchOpenTag = prevTag->Tag;
        watchCloseTag = Found.empty() ? HT_TagCount : Found.back().Tag->Tag;
        return result;
    }
    if (prevTag->Flags & TTagDescriptor::NoDescent) {
        NextTag = prevTag + 1;
        if (NextTag == Associated->Chain + Associated->NumTags) {
            Full = true;
            WasFull = true;
            result = AreaEntered;
        }
    }
    if (prevTag->Flags & TTagDescriptor::WaitForParentClose) {
        Found.push_back(TLocation(depth - 1, &fakeTag));
    }
    watchOpenTag = (NextTag == Associated->Chain + Associated->NumTags) ? HT_TagCount : NextTag->Tag;
    if (Found.empty())
        watchCloseTag = HT_TagCount;
    else {
        watchCloseTag = Found.back().Tag->Tag;
        if (Found.back().Tag->Flags & TTagDescriptor::NoDescent)
            watchOpenTag = HT_TagCount;
    }
    return result;
}
