#include <util/charset/wide.h>
#include <util/generic/buffer.h>
#include <util/memory/blob.h>
#include <util/stream/buffer.h>
#include <util/stream/buffered.h>
#include <util/stream/mem.h>
#include <util/stream/zlib.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <library/cpp/token/serialization/serialize_char_span.h>
#include <kernel/qtree/richrequest/builder/richtreebuilderaux.h>
#include <kernel/qtree/richrequest/protos/rich_tree.pb.h>
#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/qtree/richrequest/util.h>

void DeserializeMarkup(const NRichTreeProtocol::TRichRequestNode& message, TRichRequestNode& node, EQtreeDeserializeMode mode);
void SerializeMarkup(const TRichRequestNode& node, NRichTreeProtocol::TRichRequestNode& message, bool humanReadable);

void SerializeBase(NRichTreeProtocol::TRequestNodeBase& message, bool humanReadable, const TRequestNodeBase& node) {
    NRichTreeProtocol::TOpInfo* opInfo = message.MutableOpInfo();
    Y_ASSERT(opInfo);
    opInfo->SetLo(node.OpInfo.Lo);
    opInfo->SetHi(node.OpInfo.Hi);
    opInfo->SetLevel(node.OpInfo.Level);
    if (humanReadable) {
        opInfo->SetTextCmpOper(ToString(node.OpInfo.CmpOper));
        if (IsWord(node)) {
            opInfo->SetTextOp("oWord");
        } else if (IsAttribute(node)) {
            if (!!node.AttrValueHi)
                opInfo->SetTextOp("oAttrInterval");
            else
                opInfo->SetTextOp("oAttr");
        } else
            opInfo->SetTextOp(ToString(node.OpInfo.Op));
    } else {
        opInfo->SetCmpOper(node.OpInfo.CmpOper);
        if (IsWord(node)) {
            opInfo->SetOp(8);
        } else if (IsAttribute(node)) {
            if (!!node.AttrValueHi)
                opInfo->SetOp(10);
            else
                opInfo->SetOp(9);
        } else
            opInfo->SetOp(node.OpInfo.Op);
    }
    opInfo->SetArrange(node.OpInfo.Arrange);

    WideToUTF8(node.TextName, *message.MutableTextName());
    WideToUTF8(node.Text, *message.MutableText());

    // backward compatibility with the old base searches
    if (IsWordOrMultitoken(node)) {
        message.SetName(message.GetText());
        message.SetAttrValue(TString());
    } else {
        message.SetName(message.GetTextName());
        message.SetAttrValue(message.GetText());
    }
    // end of backward compatibility

    WideToUTF8(node.TextAfter, *message.MutableTextAfter());
    WideToUTF8(node.TextBefore, *message.MutableTextBefore());
    WideToUTF8(node.AttrValueHi, *message.MutableAttrValueHi());
    WideToUTF8(node.FactorName, *message.MutableFactorName());
    message.SetFactorValue(node.FactorValue);
    message.SetQuoted(node.Quoted);
    message.SetReverseFreq(node.ReverseFreq);
    if (node.GetKeyPrefix().size() == 1)
        message.SetKeyPrefix(node.GetKeyPrefix()[0]);
    else {
        for (ui32 i = 0; i < node.GetKeyPrefix().size(); ++i)
            message.AddKeyPrefixes(node.GetKeyPrefix()[i]);
    }
    message.SetWildCard(node.WildCard);
    message.SetNGrammBase(node.GetNGrammBase());
    message.SetNGrammPercent(node.GetNGrammPercent());

    if (humanReadable) {
        TString phraseType;
        try {
            phraseType = ToString(node.PhraseType);
        } catch (yexception&) {
            phraseType = ToString<int>(node.PhraseType);
        }
        message.SetTextPhraseType(phraseType);
        message.SetTextLastPhraseType(node.Quoted ? "PHRASE_QUOTE" : phraseType);
        message.SetTextNecessity(ToString(node.Necessity));
    } else {
        message.SetPhraseType(node.PhraseType);
        message.SetLastPhraseType(node.Quoted ? 4 : node.PhraseType);
        message.SetNecessity(node.Necessity);
    }

    NProto::TCharSpan* charSpan1 = message.MutableTokSpan();
    Y_ASSERT(charSpan1);
    if (node.SubTokens.size() == 1)
        SerializeCharSpan(node.SubTokens[0], *charSpan1);
    else
        SerializeCharSpan(node.GetSafeTokSpan(), *charSpan1);

    if (node.SubTokens.size() > 1) {
        for (size_t i = 0; i < node.SubTokens.size(); ++i) {
            NProto::TCharSpan* charSpan2 = message.AddSubTokens();
            Y_ASSERT(charSpan2);
            SerializeCharSpan(node.SubTokens[i], *charSpan2);
        }
    }
}

static void Serialize(const TProximity& prox, NRichTreeProtocol::TProximity& message, bool humanReadable) {
    message.SetBegin(prox.Beg);
    message.SetEnd(prox.End);
    message.SetCenter(prox.Center);
    message.SetLevel(prox.Level);
    if (humanReadable)
        message.SetTextDistanceType(ToString(prox.DistanceType));
    else
        message.SetDistanceType(prox.DistanceType);
    message.SetConvertedBounds(false);
}

void Serialize(const TRichRequestNode& node, NRichTreeProtocol::TRichRequestNode& message, bool humanReadable) {
    NRichTreeProtocol::TRequestNodeBase* nodeBase = message.MutableBase();
    Y_ASSERT(nodeBase);
    SerializeBase(*nodeBase, humanReadable, node);

    for (TNodeSequence::const_iterator i = node.Children.begin(), mi = node.Children.end(); i != mi; ++i) {
        NRichTreeProtocol::TRichRequestNode* rNode = message.AddChildren();
        Y_ASSERT(rNode);
        Serialize(*(*i), *rNode, humanReadable);
    }
    for (TRichRequestNode::TNodesVector::const_iterator i = node.MiscOps.begin(), mi = node.MiscOps.end(); i != mi; ++i) {
        NRichTreeProtocol::TRichRequestNode* rNode = message.AddMiscOps();
        Y_ASSERT(rNode);
        Serialize(*(*i), *rNode, humanReadable);
    }
    for (size_t i = 1; i < node.Children.size(); ++i) {
        NRichTreeProtocol::TProximity* prox = message.AddProxes();
        Y_ASSERT(prox);
        Serialize(node.Children.ProxBefore(i), *prox, humanReadable);
    }

    message.SetSubExpr(node.Parens);
    message.SetHiliteType(node.HiliteType);
    message.SetSnippetType(node.SnippetType);
    if (!!node.WordInfo) {
        NRichTreeProtocol::TWordNode* wordNode = message.MutableWordInfo();
        Y_ASSERT(wordNode);
        node.WordInfo->Serialize(*wordNode, humanReadable);
    }

    message.SetGeo(node.Geo);

    SerializeMarkup(node, message, humanReadable);

    if (node.FormType != fGeneral)
        message.SetFormType(node.FormType);
}

static inline int GetNew(int old) {
    if (old == 8 || old == 9 || old == 10)
        return 12;
    return old;
}

static void Deserialize(const NRichTreeProtocol::TOpInfo& message, TOpInfo& opInfo) {
    opInfo.Lo = message.GetLo();
    opInfo.Hi = message.GetHi();
    opInfo.Arrange = message.GetArrange();
    opInfo.Level = static_cast<ui16>(message.GetLevel());
    if (message.HasTextOp()) {
        TString q = message.GetTextOp();
        if (q == "oAttrInterval" || q == "oWord" || q == "oAttr")
            opInfo.Op = oUnused;
        else if (!FromString(q, opInfo.Op))
            ythrow yexception() << "invalid operation " << message.GetTextOp();
    } else {
        opInfo.Op = static_cast<TOperType>(GetNew(message.GetOp()));
    }
    if (message.HasTextCmpOper()) {
         if (!FromString(message.GetTextCmpOper(), opInfo.CmpOper))
            ythrow yexception() << "invalid cmp operation " << message.GetTextCmpOper();
    } else {
        opInfo.CmpOper = static_cast<TCompareOper>(message.GetCmpOper());
    }
}

void DeserializeBase(const NRichTreeProtocol::TRequestNodeBase& message, TRequestNodeBase& node) {
    ::Deserialize(message.GetOpInfo(), node.OpInfo);

    node.TextName = UTF8ToWide(message.GetTextName());
    node.TextAfter = UTF8ToWide(message.GetTextAfter());
    node.TextBefore = UTF8ToWide(message.GetTextBefore());
    node.Text = UTF8ToWide(message.GetText());
    node.AttrValueHi = UTF8ToWide(message.GetAttrValueHi());
    node.Quoted = message.GetQuoted();
    if (message.HasKeyPrefix())
        node.AddKeyPrefix(message.GetKeyPrefix());
    else {
        for (ui32 i = 0; i < message.KeyPrefixesSize(); ++i)
            node.AddKeyPrefix(message.GetKeyPrefixes(i));
    }
    node.SetNGrammBase(message.GetNGrammBase(), message.GetNGrammPercent());
    node.WildCard = (EWildCardFlags)message.GetWildCard();
    node.ReverseFreq = message.GetReverseFreq();
    node.FactorName = UTF8ToWide(message.GetFactorName());
    node.FactorValue = message.GetFactorValue();

    // backward compatibility with the old richtrees
    if (node.OpInfo.Op == oUnused && !node.TextName && !node.Text) {
        node.TextName = UTF8ToWide(message.GetName());
        node.Text = UTF8ToWide(message.GetAttrValue());
        if (!!node.TextName && !node.Text) {
            node.Text = node.TextName;
            node.TextName.remove();
        }
    }
    if (node.OpInfo.Op == oZone && !node.TextName) {
        node.TextName = UTF8ToWide(message.GetName());
    }
    // end of backward compatibility

    if (message.HasTextPhraseType()) {
         if (!FromString(message.GetTextPhraseType(), node.PhraseType))
            ythrow yexception() << "invalid phrase type " << message.GetTextPhraseType();
    } else {
        node.PhraseType = static_cast<TPhraseType>(message.GetPhraseType());
    }
    if (message.HasTextNecessity()) {
         if (!FromString(message.GetTextNecessity(), node.Necessity))
            ythrow yexception() << "invalid necessity " << message.GetTextNecessity();
    } else {
        node.Necessity = static_cast<TNodeNecessity>(message.GetNecessity());
    }

    for (size_t i = 0; i < message.SubTokensSize(); ++i) {
        TCharSpan spn;
        DeserializeCharSpan(spn, message.GetSubTokens(i));
        node.SubTokens.push_back(spn);
    }
    if (node.SubTokens.empty() && message.HasTokSpan()) {
        TCharSpan spn;
        DeserializeCharSpan(spn, message.GetTokSpan());
        if (!(spn == TCharSpan()))
            node.SubTokens.push_back(spn);
    }

}

static void Deserialize(TProximity& prox, const NRichTreeProtocol::TProximity& message) {
    prox.Beg = message.GetBegin();
    prox.End = message.GetEnd();
    prox.Center = message.GetCenter();
    prox.Level = static_cast<WORDPOS_LEVEL>(message.GetLevel());
    if (message.HasTextDistanceType()) {
        if (!FromString(message.GetTextDistanceType(), prox.DistanceType))
            ythrow yexception() << "invalid distance type " << message.GetTextDistanceType();
    } else {
        prox.DistanceType = static_cast<TDistanceType>(message.GetDistanceType());
    }
    message.GetConvertedBounds();
    prox.Init();
}

TRichNodePtr Deserialize(const NRichTreeProtocol::TRichRequestNode& message, EQtreeDeserializeMode mode) {
    TRichNodePtr node(new TRichRequestNode());
    DeserializeBase(message.GetBase(), *node);

    for (size_t i = 0, mi = message.ChildrenSize(); i != mi; ++i) {
        TProximity prox;
        if (i > 0 && i - 1 < message.ProxesSize())
            Deserialize(prox, message.GetProxes(i - 1));
        node->Children.Append(Deserialize(message.GetChildren(i), mode), prox);
    }
    for (size_t i = 0, mi = message.MiscOpsSize(); i != mi; ++i) {
        node->MiscOps.push_back(Deserialize(message.GetMiscOps(i), mode));
    }
    node->Parens = static_cast<bool>(message.GetSubExpr());
    node->HiliteType = static_cast<EHiliteType>(message.GetHiliteType());
    node->SnippetType = static_cast<ESnippetType>(message.GetSnippetType());

    if (message.HasWordInfo()) {
        node->WordInfo.Reset(TWordNode::CreateEmptyNode().Release());
        node->WordInfo->Deserialize(message.GetWordInfo(), mode);
    }

    node->Geo = static_cast<EGeoRuleType>(message.GetGeo());

    DeserializeMarkup(message, *node, mode);

    if (message.HasFormType())
        node->FormType = static_cast<TFormType>(message.GetFormType());
    else
        node->FormType = fGeneral;

    if (mode == QTREE_UNSERIALIZE) {
        UnUpdateRichTree(node);
    }

    return node;
}
