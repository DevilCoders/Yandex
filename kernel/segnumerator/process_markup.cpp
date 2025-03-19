#include "segmentator.h"

namespace NSegm {
namespace NPrivate {
void TSegmentator::ProcessInput(const THtmlChunk& chunk, const TZoneEntry* ze, const TNumerStat& stat) {
    if (!chunk.text || !chunk.leng)
        return;

    LinkMarkers = TLinkMarkers::New();
    FlushAttributes(stat);

    CheckTextAndReset();

    if (chunk.flags.type == PARSED_TEXT)
        return;

    ProcessMarkup(chunk, stat);

    if (ze) {
        for (size_t i = 0; i < ze->Attrs.size(); ++i)
            ProcessAttribute(ze->Attrs[i], stat);
    }
}

void TSegmentator::ProcessMarkup(const THtmlChunk& htev, const TNumerStat& stat) {
    if (!htev.Tag || htev.leng <= 2)
        return;

    const NHtml::TTag* tag = htev.Tag;
    const HT_TAG httag = tag->id();
    const bool empty = tag->flags & HT_empty; //the tag is empty
    const bool invalid = MARKUP_IGNORED == htev.flags.markup; //the tag is discarded
    const bool close = '/' == htev.text[1]; //the tag is closing
    const bool selfclose = '/' == htev.text[htev.leng - 2];

        // Extracting descritpion from meta (opengraph, facebook, etc.)
    if (HT_META == httag) {
        TStringBuf description;
        bool IsName = false;
        for (size_t i = 0; i < htev.AttrCount; i++) {
            const NHtml::TAttribute& attr = htev.Attrs[i];
            TStringBuf name = TStringBuf(htev.text + attr.Name.Start, attr.Name.Leng);
            TStringBuf value = TStringBuf(htev.text + attr.Value.Start, attr.Value.Leng);
            if (name == "name" &&
                   (value == "description" ||
                    value == "og:description" ||
                    value == "fb:description"))
            {
                IsName = true;
            }
            if (name == "content") {
                description = value;
            }
        }
        if (IsName && description.size() > MetaDescription.size()) {
            MetaDescription = description;
        }
    }

    //Processing bolds, closing blockbolds if open.
    //Bolds are always ignored although do have effect on boldness.
    BoldDistance.RenewIrregular(httag, close);

    if (!selfclose || close)
        CheckIgnoreContent(httag, close);

    //Discarding other ignored markup
    if (invalid)
        return;

    //bold spans
    if (HT_SPAN == httag) {
        BoldDistance.RenewSpan(SpanCount, close);

        if (close) {
            if (SpanCount)
                SpanCount--;
        } else {
            AttributeExpectation = ExpectAttrSpan;
            SpanCount++;
        }

        return;
    }

    if (!ZeroWeight) {
        //Processing area
        if (HT_AREA == httag) {
            AttributeExpectation = ExpectAttrArea;
            return;
        } else if (HT_IMG == httag) {
            AttributeExpectation = ExpectAttrImg;
            return;
        }

        //Processing ancor
        if (HT_A == httag) {
            // mark link as implied for proper statistics calculation for segment
            if (Html5Parser && htev.flags.markup == MARKUP_IMPLIED) {
                LinkMarkers.Implied = 1;
            }
            if (close)
                CloseLinkNode(stat);
            else
                AttributeExpectation = ExpectAttrAncor;
            return;
        }

        //Processing inline
        if (!close && IsInputTag(httag)) {
            AddInput(stat);
            return;
        }

        if (empty && IsBreakTag(httag)) {
            AddBreak(stat, httag);
            return;
        }
    }

    //Processing blocks
    if (IsStructTag(httag)) {
        if (close) {
            CloseBlockNode(stat, httag);
            return;
        }

        const HT_TAG lastTag = Ctx->BlockStack.back().Tag;
        bool closeParent = false;

        // TODO: remove workarounds for fixed bugs

        if (!Html5Parser) { // skip tree fixing workarounds with new html5 parser
            /*workaround for jira:ARC-841 (dangling implied <ul> or <dl>)*/
            if (MARKUP_IMPLIED == htev.flags.markup && (HT_UL == httag || HT_DL == httag))
                return;

            /*workaround for jira:ARC-842 (blocks other than <p> inside <address>)*/
            if (HT_ADDRESS == lastTag && HT_P != httag)
                closeParent = true;

            /*workaround for jira:ARC-843 (<h1..6> inside <h1..6>)*/
            if (IsHxTag(httag) && IsHxTag(lastTag))
                closeParent = true;
        }
        OpenBlockNode(stat, httag, closeParent);
    }
}

void TSegmentator::OpenBlockNode(const TNumerStat& stat, HT_TAG tag, bool closeLast) {
    if (closeLast)
        CloseBlockNode(stat, CurrentBlockNode()->Props.Tag);

    BoldDistance.Move();

    /*workaround for jira:ARC-911 (<table> inside <a>) or skip fix with new html5 parser*/
    if (!Html5Parser) {
        if (HT_TABLE == tag)
            CloseLinkNode(stat);
    }

    if (IgnoredHierarchy()) {
        Ctx->BlockStack.push_back(TTagItem(tag));
        AttributeExpectation = ExpectAttrInlineBlock;
        AddBreak(stat, tag);

        if (IsHxTag(tag))
            BoldDistance.OpenBoldBlock(tag);
    } else {
        if (MaxNodeCount <= NodeCount)
            return;

        NodeCount++;

        Y_VERIFY(IsA<DNT_BLOCK> (CurrentNode), " ");
        CurrentNode = MakeBlock(Ctx->Pool, stat.TokenPos.Pos, CurrentNode, tag);
        Ctx->BlockStack.push_back(TTagItem(tag, CurrentNode));
        AttributeExpectation = ExpectAttrBlock;

        Storer->OnBlockOpen(tag, stat.TokenPos.Pos);
    }
}

void TSegmentator::CloseBlockNode(const TNumerStat& stat, HT_TAG tag) {
    if (tag != Ctx->BlockStack.back().Tag)
        return;

    if (Ctx->BlockStack.back().Block) {
        /*in case the ancor close event has been ignored*/
        if (IsA<DNT_LINK> (CurrentNode))
            CloseLinkNode(stat);

        Y_VERIFY(Ctx->BlockStack.back().Block == CurrentNode, " ");
        CurrentNode->GenerateSignature();
        CurrentNode = CurrentNode->CloseList(stat.TokenPos.Pos);

        Storer->OnBlockClose(tag, stat.TokenPos.Pos);
    } else {
        AddBreak(stat, tag);
        BoldDistance.RenewBlock(tag, true);
    }

    BoldDistance.Move(true);
    Ctx->BlockStack.pop_back();
}

void TSegmentator::OpenLinkNode(const TNumerStat& stat, const TString& url) {
    CloseLinkNode(stat);

    if (MaxNodeCount <= NodeCount)
        return;

    NodeCount++;

    ui32 hosthash = 0;
    ELinkType lt = Ctx->OwnerInfo.CheckLink(url, hosthash);
    CurrentNode = MakeLink(Ctx->Pool, stat.TokenPos.Pos, CurrentNode, lt, hosthash);
    CurrentNode->Props.LinkMarkers |= LinkMarkers;
    Storer->OnLinkOpen(url, lt, stat.TokenPos.Pos);
}

void TSegmentator::CloseLinkNode(const TNumerStat& stat) {
    BoldDistance.CloseAncor();
    if (IsA<DNT_LINK> (CurrentNode)) {
        CurrentNode = CurrentNode->CloseList(stat.TokenPos.Pos);
        Storer->OnLinkClose(stat.TokenPos.Pos);
    }
}

void TSegmentator::AddBreak(const TNumerStat& stat, HT_TAG tag) {
    TDocNode* last = CurrentBreakNode();

    if (last) {
        last->MergeBreak(tag);
    } else {
        if (MaxNodeCount <= NodeCount)
            return;

        NodeCount++;

        MakeBreak(Ctx->Pool, stat.TokenPos.Pos, CurrentNode, tag);
    }

    Storer->OnBreak(tag, stat.TokenPos.Pos);
}

void TSegmentator::AddInput(const TNumerStat& stat) {
    TDocNode* last = CurrentInputNode();

    if (last) {
        last->Props.NInputs += 1;
    } else {
        if (MaxNodeCount <= NodeCount)
            return;

        NodeCount++;

        MakeInput(Ctx->Pool, stat.TokenPos.Pos, CurrentNode, 1);
    }

    Storer->OnInput(stat.TokenPos.Pos);
}

}
}
