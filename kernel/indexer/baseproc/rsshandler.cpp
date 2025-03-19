#include "rsshandler.h"

void TRssNumeratorHandler::Clear() {
    DivCount = 0;
    State = STATE_NORMAL;
    Link = Description = Date = Anchor = Author = "";
}

void TRssNumeratorHandler::Store() {
    TRssLinkRec rec;

    if (Link.size() + 1 >= sizeof(rec.Link)) {
        return;
    }
    urlid_t uid(0, 0);
    if (!GetHostIdUrlIdByUrl(Link.data(), uid)) {
        return;
    }

    if (Date.size() && uid.HostId == HostId && uid.HostId != ui32(-1)) {
        time_t timestamp;
        bool parsed = ParseRFC822DateTimeDeprecated(Date.data(), timestamp) || sscanf(Date.data(), "%zu", &timestamp) == 1;

        if (parsed) {
            DateRecords.push_back(TRssDateRec(uid, timestamp));
        }
    }

    memset(rec.Link, 0, sizeof(rec.Link));
    strcpy(rec.Link, Link.data());

    TRssLinkRec::TExtInfo extInfo;

    extInfo.SetDescription(Description.data());
    extInfo.SetDate(Date.data());
    extInfo.SetAnchor(Anchor.data());
    extInfo.SetAuthor(Author.data());
    extInfo.SetSource(Source.data());

    Records.push_back(rec);
    ExtRecords.push_back(extInfo);
}

bool TRssNumeratorHandler::CheckAttr(const NHtml::TAttribute* attr, const char* attrName, const char* attrValue, const char* text) {
    if (attr->IsBoolean()) {
        return false;
    }

    const char* curAttrName  = text + attr->Name.Start;
    const char* curAttrValue = text + attr->Value.Start;
    ui32 attrNameLen  = attr->Name.Leng;
    ui32 attrValueLen = attr->Value.Leng;

    return (strlen(attrName) == attrNameLen &&
            strlen(attrValue) == attrValueLen &&
            strncmp(attrName, curAttrName, attrNameLen) == 0 &&
            strncmp(attrValue, curAttrValue, attrValueLen) == 0);
}

bool TRssNumeratorHandler::CheckTag(const THtmlChunk& e, HT_TAG tag, bool isClose) {
    if (isClose) {
        return
            e.GetLexType() == HTLEX_END_TAG &&
            e.Tag->id() == tag &&
            e.AttrCount == 0;
    } else {
        return
            e.GetLexType() == HTLEX_START_TAG &&
            e.Tag->id() == tag &&
            e.AttrCount == 1;

    }
}

void TRssNumeratorHandler::SetFromAttr(TString& target, const NHtml::TAttribute* attr, const char* text) {
    target = TString(text + attr->Value.Start, attr->Value.Leng);
}

bool TRssNumeratorHandler::CheckLink(const THtmlChunk& e) {
    if (e.GetLexType() != HTLEX_START_TAG ||
        e.Tag->id() != HT_A ||
        e.AttrCount != 2) {
        return false;
    }

    const NHtml::TAttribute* firstAttr = e.Attrs;
    const NHtml::TAttribute* secondAttr = e.Attrs + 1;

    static const char href[] = "href";
    static const char pubdate[] = "pubdate";

    if (strncmp(href, e.text + firstAttr->Name.Start, firstAttr->Name.Leng) != 0 ||
        strncmp(pubdate, e.text + secondAttr->Name.Start, secondAttr->Name.Leng) != 0) {
        return false;
    }

    SetFromAttr(Link, firstAttr, e.text);
    SetFromAttr(Date, secondAttr, e.text);
    return true;
}

bool TRssNumeratorHandler::CheckText(const THtmlChunk& e, TString* target) {
    if (e.GetLexType() != HTLEX_TEXT) {
        return false;
    }

    if (target) {
        *target = TString(e.text, e.leng);
    }

    return true;
}

bool TRssNumeratorHandler::Check(const THtmlChunk& e, HT_TAG tag, const char* attrName, const char* attrValue) {
    return
        CheckTag(e, tag) &&
        CheckAttr(e.Attrs, attrName, attrValue, e.text);
}

void TRssNumeratorHandler::ProcessNormal(const THtmlChunk& e) {
    if (Check(e, HT_LI, "class", "item")) {
        State = STATE_LI;
    }
}

void TRssNumeratorHandler::ProcessLi(const THtmlChunk& e) {
    if (Check(e, HT_H4, "class", "itemtitle")) {
        State = STATE_H4;
    } else {
        Clear();
    }
}

void TRssNumeratorHandler::ProcessH4(const THtmlChunk& e) {
    if (CheckLink(e)) {
        State = STATE_ANCHOR;
    } else {
        Clear();
    }
}

void TRssNumeratorHandler::ProcessAnchor(const THtmlChunk& e) {
    TString buf;
    if (CheckTag(e, HT_A, true)) {
        State = STATE_A_END;
    } else if (CheckText(e, &buf)) {
        Anchor += buf;
    } else {
        Clear();
    }
}

void TRssNumeratorHandler::ProcessAEnd(const THtmlChunk& e) {
    if (CheckTag(e, HT_H4, true)) {
        State = STATE_H4_END;
    } else {
        Clear();
    }
}

void TRssNumeratorHandler::ProcessH4End(const THtmlChunk& e) {
    if (Check(e, HT_DIV, "class", "date")) {
        State = STATE_DATE;
        DivCount = 1;
    } else {
        Clear();
    }
}

void TRssNumeratorHandler::ProcessDate(const THtmlChunk& e) {
    if (CheckTag(e, HT_DIV, false)) {
        ++DivCount;
    } else if (CheckTag(e, HT_DIV, true)) {
        --DivCount;
        if (DivCount == 0) {
            State = STATE_DIV_DATE_END;
        }
    }
}

void TRssNumeratorHandler::ProcessDivDateEnd(const THtmlChunk& e) {
    if (Check(e, HT_DIV, "class", "itemcontent")) {
        State = STATE_ITEMCONTENT;
        DivCount = 1;
    } else {
        Clear();
    }
}

void TRssNumeratorHandler::ProcessItemContent(const THtmlChunk& e) {
    TString buf;
    if (CheckTag(e, HT_DIV, false)) {
        ++DivCount;
    } else if (CheckTag(e, HT_DIV, true)) {
        --DivCount;
        if (DivCount == 0) {
            State = STATE_DIV_ITEMCONTENT_END;
        }
    } else if (CheckText(e, &buf)) {
        Description += buf;
    }
}

void TRssNumeratorHandler::ProcessDivItemContentEnd(const THtmlChunk& e) {
    if (Check(e, HT_DIV, "class", "author")) {
        State = STATE_AUTHOR;
        DivCount = 1;
    } else {
        Clear();
    }
}

void TRssNumeratorHandler::ProcessAuthor(const THtmlChunk& e) {
    TString buf;
    if (CheckTag(e, HT_DIV, false)) {
        ++DivCount;
    } else if (CheckTag(e, HT_DIV, true)) {
        --DivCount;
        if (DivCount == 0) {
            State = STATE_DIV_AUTHOR_END;
        }
    } else if (CheckText(e, &buf)) {
        Author += buf;
    }
}

void TRssNumeratorHandler::ProcessDivAuthorEnd(const THtmlChunk& e) {
    if (CheckTag(e, HT_LI, true)) {
        Store();
    }
    Clear();
}

TRssNumeratorHandler::TRssNumeratorHandler(ui32 hostId, THostsTable* hosts, const TString& source)
    : Source(source)
    , HostId(hostId)
    , HostTable(hosts)
{
    Handlers.push_back(&TRssNumeratorHandler::ProcessNormal);
    Handlers.push_back(&TRssNumeratorHandler::ProcessLi);
    Handlers.push_back(&TRssNumeratorHandler::ProcessH4);
    Handlers.push_back(&TRssNumeratorHandler::ProcessAnchor);
    Handlers.push_back(&TRssNumeratorHandler::ProcessAEnd);
    Handlers.push_back(&TRssNumeratorHandler::ProcessH4End);
    Handlers.push_back(&TRssNumeratorHandler::ProcessDate);
    Handlers.push_back(&TRssNumeratorHandler::ProcessDivDateEnd);
    Handlers.push_back(&TRssNumeratorHandler::ProcessItemContent);
    Handlers.push_back(&TRssNumeratorHandler::ProcessDivItemContentEnd);
    Handlers.push_back(&TRssNumeratorHandler::ProcessAuthor);
    Handlers.push_back(&TRssNumeratorHandler::ProcessDivAuthorEnd);
    Clear();
}

bool TRssNumeratorHandler::GetHostIdUrlIdByUrl(const TString& url, urlid_t& result) {
    result.HostId = -1;
    TString cur = url;

    if (GetHttpPrefixSize(url.data()) == 0) {
        cur = TString::Join("http://", url.data());
    }

    THttpURL parsed;
    THttpURL::TParsedState state = parsed.ParseUri(cur);

    if (!HostTable)
        return state == THttpURL::ParsedOK;

    bool good = state == THttpURL::ParsedOK && HostTable->LookupLocalHostId(parsed.GetField(THttpURL::FieldHost).data(), result.HostId);
    if (good) {
        UrlHashVal(result.UrlId, parsed.GetField(THttpURL::FieldPath).data());
    }
    return good;
}

bool TRssNumeratorHandler::IsTrash(const THtmlChunk& e) {
    return e.GetLexType() == HTLEX_EOF;
}

void TRssNumeratorHandler::OnAddEvent(const THtmlChunk& e) {
    if (IsTrash(e)) {
        return;
    }

    TStateHandler handler = Handlers[ui32(State)];

    (this ->* handler)(e);
}

void TRssNumeratorHandler::StoreResult(TOutDatFile<TRssLinkRec>& outputLinks, TDatSorterMemo<TRssDateRec, ByUid>& outputDates) const {
    if (Records.size() != ExtRecords.size()) {
        ythrow yexception() << "Error in prewalrus - extract rss links. Count of records doesn't equal to count of ext records!";
    }

    for (size_t index = 0; index < Records.size(); ++index) {
        outputLinks.Push(&Records[index], &ExtRecords[index]);
    }
    for (size_t index = 0; index < DateRecords.size(); ++index) {
        outputDates.Push(&DateRecords[index]);
    }
}

void TRssNumeratorHandler::StoreResult(TOutDatFile<TRssLinkRec>& links, TOutDatFile<TRssDateRec>& dates) const {
    if (Records.size() != ExtRecords.size())
        ythrow yexception() << "Error in prewalrus - extract rss links. Count of records doesn't equal to count of ext records!";

    for (size_t i = 0; i < Records.size(); ++i)
        links.Push(&Records[i], &ExtRecords[i]);

    for (size_t i = 0; i < DateRecords.size(); ++i)
        dates.Push(&DateRecords[i]);
}

