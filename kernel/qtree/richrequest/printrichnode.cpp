#include "nodeiterator.h"
#include "richnode.h"
#include "wordnode.h"
#include "printrichnode.h"
#include "faststringbuilder.h"

#include <kernel/keyinv/invkeypos/keycode.h>

#include <util/string/util.h>
#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>

namespace NPrivate {
    using namespace NSearchQuery;

    const int flNEAR_PHRASE = 1;

    //-------------------------- LEX constants: ----------------------------

    // not used values were commented out
    //const wchar16 TITLE_STR[] = { 't', 'i', 't', 'l', 'e', 0 };
    const wchar16 INPOS_STR[] = { 'i', 'n', 'p', 'o', 's', 0 };
    const wchar16 SOFTNESS_STR[] = { 's', 'o', 'f', 't', 'n', 'e', 's', 's', 0 };
    const wchar16 REFINE_FACTOR_STR[] = { 'r', 'e', 'f', 'i', 'n', 'e', 'f', 'a', 'c', 't', 'o', 'r', 0 };

    const wchar16 COLON_STR = ':';

    const wchar16 LEFT_PAREN_STR = '(';
    const wchar16 RIGHT_PAREN_STR  = ')';

    const wchar16 AND_STR = '&';
    const wchar16 AND_NOT_STR = '~';
    const wchar16 OR_STR = '|';
    const wchar16 WEAK_OR_STR = '^';
    const wchar16 REFINE_STR[] = {'<', '-', 0};
    const wchar16 RESTR_DOC_STR[] = {'<', '<', 0};

    const wchar16 NEAR_PREFFIX_STR = '/';
    const wchar16 REVERSE_FREQ_PREFFIX_STR[] = {':', ':', 0};

    //const wchar16 CMP_LE_STR[] = {'<', '=', 0};
    //const wchar16 CMP_GE_STR[] = {'>', '=', 0};

    const wchar16 NEW_CMP_LT_STR[] = {':', '<', 0};
    const wchar16 NEW_CMP_LE_STR[] = {':', '<', '=', 0};
    const wchar16 NEW_CMP_EQ_STR[] = {':', 0};
    const wchar16 NEW_CMP_GE_STR[] = {':', '>', '=', 0};
    const wchar16 NEW_CMP_GT_STR[] = {':', '>', 0};

    const wchar16 EXACT_WORD_PREFFIX_STR = '!';
    const wchar16 EXACT_LEMMA_PREFFIX_STR[] = {'!', '!', 0};
    //const wchar16 BEG_ATTRVALUE_STR = '\"';
    const wchar16 NEAR_DELIM_STR = '\"';

    //const wchar16 BEG_RESTRBYPOS_STR[] = {'[', '[', 0};
    //const wchar16 END_RESTRBYPOS_STR[] = {']', ']', 0};
    //const wchar16 COMMA_STR[] = {',', ',', 0};


    //new params for new syntax
    const wchar16 CMP_RIGHT_ARROW[] = {'-', '>', 0};
    const wchar16 NEW_DOUBLE_DOT[] = {'.', '.', 0};
    //

    const wchar16 SPACE_STR = ' ';

    const wchar16 MUST_STR = '+';

    const wchar16 LEFT_BRACE_STR = '{';
    const wchar16 RIGHT_BRACE_STR  = '}';

    const wchar16 EQUAL_STR = '=';

    inline bool IsQuoteChar(wchar16 q) {
        return q == '\"' || q == '\'' || q == '`';
    }

    //----------------------------------------------------------------------
    //TRichRequestCollector

    class TRichRequestCollector {
    private:
        const TRichRequestNode* Root;
        ui32 Softness;
        EPrintRichRequestOptions Options;

        TFastWtringAsciiBuilder Request;

        bool PrintingZoneAttr; //флаг - начали писать oZone.
        bool PrintingZone;     //флаг - начали писать аттрибут, принадлежащий зоне.
        bool DropNextSpace;    //флаг - пишем мультитокен, следующий пробел не нужен

        void PrintNearSpec(int lo, int hi, int level);
        void PrintRestrictBounds(const TOpInfo& op);
        void PrintBinOp(int lo, int hi, int level, TDistanceType distance,
            const TRichRequestNode* n);
        void PrintReverseFreq(const TRichRequestNode* n);
        void PrintSoftness(ui32 s);
        void PrintCmpOp(const TRichRequestNode* n);
        void PrintNecessity(const TRichRequestNode* n, TNodeNecessity parN);
        TMaybe<TFormType> PrintFormType(const TRichRequestNode* n, TMaybe<TFormType> parFT);
        void PrintWord(const TRichRequestNode* n, TMaybe<TFormType> parFT, TNodeNecessity parN, bool printMetaData);
        void PrintPreRestrDoc(const TRichRequestNode* n, TMaybe<TFormType> parFT, TNodeNecessity parN);
        void PrintPostRestrDoc(const TRichRequestNode* n);
        void PrintBinOp(const TRichRequestNode* n, int qNearPhrase, TMaybe<TFormType> parFT, TNodeNecessity parN, bool printMetaData);
        void PrintZone(const TRichRequestNode* n, TNodeNecessity& parN,
                       const TRichRequestNode* parent, bool start);
        void PrintRestrictByPos(const TRichRequestNode* n, bool start);
        void PrintAttr(const TRichRequestNode* n, TMaybe<TFormType> parFT, TNodeNecessity parN);
        void PrintAttrInterval(const TRichRequestNode* n, TNodeNecessity parN);
        void PrintPreMiscOps(const TRichRequestNode* n, int qNearPhrase, TMaybe<TFormType>& parFT, TNodeNecessity& parN);
        void PrintPostMiscOps(const TRichRequestNode* n, int qNearPhrase, TMaybe<TFormType> parFT, TNodeNecessity parN);
        void PrintQuota(const TRichRequestNode* n);
        void PrintEndOfSynonymGroup(const TRichRequestNode* n, size_t childPos);
        void PrintBeginOfSynonymGroup(const TRichRequestNode* n, size_t childPos);

        void PrintRichRequest(const TRichRequestNode* n, int qNearPhrase = 0,
                              TMaybe<TFormType> parFT = fGeneral, TNodeNecessity parN = nDEFAULT,
                     const TRichRequestNode* parent = nullptr, bool start = false);
        //! @todo remove copy-paste, see arcadia\ysite\yandex\request\printreq.h, needParen()
        bool NeedParen(const TRichRequestNode* n) const;
        //! @todo remove copy-paste, see arcadia\ysite\yandex\request\printreq.h, isBinOp()
        bool IsBinOp(const TRichRequestNode* n) const;
        //! @todo remove copy-paste, see arcadia\ysite\yandex\request\printreq.h, needModifier()
        bool NeedModifier(const TRichRequestNode* n) const;
        bool IsPrintRestrDoc(const TRichRequestNode* n);
    public:
        TRichRequestCollector(const TRichRequestNode* n, ui32 s, EPrintRichRequestOptions options);
        TUtf16String PrintRichRequest();
    };


    TRichRequestCollector::TRichRequestCollector(const TRichRequestNode* root, ui32 s, EPrintRichRequestOptions options)
        : Root(root)
        , Softness(s)
        , Options(options)
        , PrintingZoneAttr(false)
        , PrintingZone(false)
        , DropNextSpace(false)
    {
    }


    TUtf16String TRichRequestCollector::PrintRichRequest() {
        Request.Reset();
        if (Root) {
            PrintingZone = false;
            PrintingZoneAttr = false;
            DropNextSpace = false;
            PrintRichRequest(Root);
            if (Softness && !Root->Children.empty())
                PrintSoftness(Softness);
        }

        return Request.Buffer();
    }

    bool TRichRequestCollector::NeedParen(const TRichRequestNode* n) const {
        return n->Parens;
    }

    bool TRichRequestCollector::IsBinOp(const TRichRequestNode* n) const {
        return IsAndOp(*n) || n->Op() == oAndNot || n->Op() == oOr ||
            n->Op() == oRefine || n->Op() == oWeakOr ||
            n->Op() == oRestrDoc;
    }

    bool TRichRequestCollector::NeedModifier(const TRichRequestNode* n) const {
        return !IsBinOp(n) || NeedParen(n) || n->IsQuoted();
    }


    void TRichRequestCollector::PrintNearSpec(int lo, int hi, int level) {
        if (lo || hi || !level) {
            Request << NEAR_PREFFIX_STR << LEFT_PAREN_STR << (long)lo << SPACE_STR << (long)hi << RIGHT_PAREN_STR;
        }
    }


    void TRichRequestCollector::PrintRestrictBounds(const TOpInfo& op) {
        Request << SPACE_STR << INPOS_STR << COLON_STR<< (long)op.Lo << NEW_DOUBLE_DOT << (long)op.Hi;
    }

    void TRichRequestCollector::PrintBinOp(int lo, int hi, int level, TDistanceType distance,
        const TRichRequestNode* n) {
        size_t l = level;

        switch (n->OpInfo.Op) {
        case oAnd:
        case oAndNot:
            if (Options & PRRO_ProxInfo) {
                Request << LEFT_BRACE_STR << (int)lo << SPACE_STR << (int)hi << SPACE_STR << (int)level << SPACE_STR << (int)distance << RIGHT_BRACE_STR;
            }
            if ((lo || hi || !level))
                ++l;
            for (size_t i = 0; i < l; ++i)
                Request << (n->OpInfo.Op == oAnd ? AND_STR : AND_NOT_STR);
            PrintNearSpec(lo, hi, level);
            break;
        case oOr:
            Request << OR_STR;
            break;
        case oWeakOr:
            Request << WEAK_OR_STR;
            break;
        case oRefine:
            Request << REFINE_STR;
            if (!n->GetFactorName().empty())
                Request << SPACE_STR << REFINE_FACTOR_STR << COLON_STR << n->GetFactorName() << EQUAL_STR << n->GetFactorValue();
            break;
        case oRestrDoc:
            Request << RESTR_DOC_STR;
            break;
        default:
            break;
        }
    }

    void TRichRequestCollector::PrintReverseFreq(const TRichRequestNode* n) {
        if ((n->ReverseFreq >= 0) && !(Options & PRRO_RevFreqs))
            Request << REVERSE_FREQ_PREFFIX_STR << (long)n->ReverseFreq;
        if (Options & PRRO_StopWords)
            if (n->WordInfo->IsStopWord())
                Request << u"[SW]";
    }

    void TRichRequestCollector::PrintSoftness(ui32 s) {
        Request << SPACE_STR << SOFTNESS_STR << COLON_STR << (unsigned long)s;
    }


    void TRichRequestCollector::PrintCmpOp(const TRichRequestNode* n) {
        switch (n->OpInfo.CmpOper) {
        case cmpLT: Request << NEW_CMP_LT_STR; break;
        case cmpLE: Request << NEW_CMP_LE_STR; break;
        case cmpEQ: Request << NEW_CMP_EQ_STR; break;
        case cmpGE: Request << NEW_CMP_GE_STR; break;
        case cmpGT: Request << NEW_CMP_GT_STR; break;
        default:    Y_ASSERT(0);
        }
    }

    void TRichRequestCollector::PrintNecessity(const TRichRequestNode* n, TNodeNecessity parN) {
        if (!NeedModifier(n))
            return;
        if (n->Necessity != parN){
            if (n->Necessity == nMUST)
                Request << MUST_STR;
        }
    }

    class TCommonFormTypeCollector {
    private:
        bool TreeHasCommonFormType = true;
        TMaybe<TFormType> FormType;
    public:
        TMaybe<TFormType> GetCommonFormType() const {
            return TreeHasCommonFormType ? FormType : TMaybe<TFormType>();
        }

        void CollectFormType(const TRichRequestNode* n){
            if (IsWord(*n)){
                if (!n->WordInfo.Get())
                    ythrow yexception() << "No WordInfo in TCommonFormTypeCollector::CollectFormType.";
                if (!FormType){
                    FormType = n->WordInfo->GetFormType();
                } else {
                    if (FormType != n->WordInfo->GetFormType())
                        TreeHasCommonFormType = false;
                }
            } else
                for (size_t i = 0; i < n->Children.size(); ++i)
                    CollectFormType(n->Children[i].Get());
        }
    };


    TMaybe<TFormType> TRichRequestCollector::PrintFormType(const TRichRequestNode* n, TMaybe<TFormType> parFT) {
        if (!NeedModifier(n))
            return parFT;

        TMaybe<TFormType> ft;

        if (!!n->WordInfo){
            if (n->WordInfo->GetFormType() != parFT)
                ft = n->WordInfo->GetFormType();
        } else {
            TCommonFormTypeCollector fft;
            fft.CollectFormType(n);
            if (fft.GetCommonFormType() != parFT)
                ft = fft.GetCommonFormType();
        }

        if (ft == fExactLemma)
            Request << EXACT_LEMMA_PREFFIX_STR;
        else if (ft == fExactWord)
            Request << EXACT_WORD_PREFFIX_STR;
        if (!IsWord(*n) && ft)
            return ft;

        return parFT;
    }

    void TRichRequestCollector::PrintWord(const TRichRequestNode* n, TMaybe<TFormType> parFT,
                                      TNodeNecessity parN, bool printMetaData) {
        PrintFormType(n, parFT);
        if (printMetaData)
            PrintNecessity(n, parN);

        PrintQuota(n);
        const TUtf16String& text = n->GetText();
        if (text[0] == PUNCT_PREFIX && text.size() == PUNCT_PREFIX_LEN && isxdigit(text[1]) && isxdigit(text[2])) {
            Request << DecodeWordPrefix(text.c_str(), text.size());
        } else {
            Request << text;
        }
        PrintQuota(n);
        PrintReverseFreq(n);

        DropNextSpace = false;

        const TUtf16String& punct = n->GetPunctAfter();

        if (!punct.empty() && !(Options & PRRO_IgnorePunct)) {
            const TUtf16String& text_after = n->GetTextAfter();
            // Hack to re-merge common multitokens
            if (text_after.length() == 1 && (text_after[0] == '-' || text_after[0] == '\'' || text_after[0] == '/'))
                DropNextSpace = true;

            Request << punct;
        }
    }


    void TRichRequestCollector::PrintPostRestrDoc(const TRichRequestNode* n){
        if (NeedParen(n))
            Request << RIGHT_PAREN_STR;
    }

    void TRichRequestCollector::PrintPreRestrDoc(const TRichRequestNode* n,
                                                 TMaybe<TFormType> parFT, TNodeNecessity parN){
        PrintNecessity(n, parN);
        PrintFormType(n, parFT);
        if (NeedParen(n))
            Request << LEFT_PAREN_STR;
    }

    void TRichRequestCollector::PrintQuota(const TRichRequestNode* n){
        if (n->IsQuoted())
        Request << NEAR_DELIM_STR;
    }

    void TRichRequestCollector::PrintBeginOfSynonymGroup(const TRichRequestNode* n, size_t childPos) {
        size_t oneWordSynonymsCount = 0;
        for (TForwardMarkupIterator<TSynonym, true> it(n->Markup()); !it.AtEnd(); ++it)
            if (it->Range.Beg == childPos) {
                if (GetRangeLength(it->Range) > 1)
                    Request << LEFT_PAREN_STR;
                else {
                    if (0 == oneWordSynonymsCount) {
                        Request << LEFT_PAREN_STR;
                        ++oneWordSynonymsCount;
                    }
                }
            }
    }


    namespace {
        struct TSynonymSorter {
            bool operator()(const TMarkupItem& left, const TMarkupItem& right) {
                return GetRangeLength(left.Range) < GetRangeLength(right.Range);
            }
        };
    }  // namespace

    void TRichRequestCollector::PrintEndOfSynonymGroup(const TRichRequestNode* n, size_t childPos) {
        TMarkup::TItems sortedSynonyms;
        for (TForwardMarkupIterator<TSynonym, true> it(n->Markup()); !it.AtEnd(); ++it) {
            if (it->Range.End == childPos)
                sortedSynonyms.push_back(*it);
        }
        std::sort(sortedSynonyms.begin(), sortedSynonyms.end(), TSynonymSorter());

        size_t oneWordSynonymsCount = 0;

        for (size_t i = 0; i != sortedSynonyms.size(); ++i) {
            if (GetRangeLength(sortedSynonyms[i].Range) > 1) {
                if (oneWordSynonymsCount) {
                    Request << RIGHT_PAREN_STR;
                    oneWordSynonymsCount = 0;
                }
                Request << RIGHT_PAREN_STR;
            } else
                ++oneWordSynonymsCount;

            Request << SPACE_STR << WEAK_OR_STR << SPACE_STR;
            if (!sortedSynonyms[i].GetDataAs<TSynonym>().SubTree->Children.empty())
                Request << LEFT_PAREN_STR;
            PrintRichRequest(sortedSynonyms[i].GetDataAs<TSynonym>().SubTree.Get(), 0, fGeneral, nDEFAULT, nullptr);
            if (!sortedSynonyms[i].GetDataAs<TSynonym>().SubTree->Children.empty())
                Request << RIGHT_PAREN_STR;
        }
        if (oneWordSynonymsCount > 0) {
            Request << RIGHT_PAREN_STR;
        }
    }


    bool TRichRequestCollector::IsPrintRestrDoc(const TRichRequestNode* n) {
        if (oRestrDoc == n->Op()) {
            if  (IsAttribute(*n->Children[0]))   //word1 attr:(word1) - don't print "oRestrDoc" with attribute
                return false;
            return true;
        }
        return false;
    }

    void TRichRequestCollector::PrintBinOp(const TRichRequestNode* n, int qNearPhrase,
                                           TMaybe<TFormType> parFT, TNodeNecessity parN, bool printMetaData){
        TMaybe<TFormType> ft = fGeneral;
        if ((oRestrDoc != n->Op()) && (oRefine != n->Op()) && (oAndNot != n->Op())){
            PrintNecessity(n, parN);
            ft = PrintFormType(n, parFT);
            if (NeedParen(n) && PrintingZoneAttr) {
                Request << LEFT_PAREN_STR;
        }
        } else {
            Request << SPACE_STR;
        }
        bool nearPhrase = n->IsQuoted();

        int qChildNearPhrase = qNearPhrase;
        if (nearPhrase)
            qChildNearPhrase |= flNEAR_PHRASE;

        if (n->Children.empty()) {
            ythrow yexception() << "invalid tree format";
        }

        //печатаем бинарную операцию, первый ребенок уже напечатан.
        if ((IsPrintRestrDoc(n)) || (oAndNot == n->Op()) || (oRefine == n->Op()))
            if ((qChildNearPhrase & flNEAR_PHRASE)== 0){
                if (n->Children.size() > 1) {
                    const TProximity* prx = &n->Children.ProxAfter(0);  //оператор между двумя детьми
                    PrintBinOp(prx->Beg, prx->End, prx->Level - 3, prx->DistanceType, n);
                } else {
                    PrintBinOp(n->OpInfo.Lo, n->OpInfo.Hi, n->OpInfo.Level, DT_NONE, n);
                }
                Request << SPACE_STR;
            }

        TVector<TRichNodePtr>::const_iterator it, end;
        it = n->Children.begin();
        end = n->Children.end();
        --end;

        if (NeedParen(n) && !PrintingZoneAttr)
            Request << LEFT_PAREN_STR;

        PrintQuota(n);

        size_t pos = 0;
        //пишем детей
        for (; it != end; ++it, ++pos){
            const TProximity* prx = &n->Children.ProxAfter(pos);
            if (printMetaData)
                PrintBeginOfSynonymGroup(n, pos);
            PrintRichRequest((*it).Get(), qChildNearPhrase, ft, n->Necessity, nullptr);
            if (printMetaData)
                PrintEndOfSynonymGroup(n, pos);

            if (!DropNextSpace || !IsAndOp(*n) || !IsMultitoken(*n) && !n->IsQuoted()) {
                Request << SPACE_STR;
                if ((qChildNearPhrase & flNEAR_PHRASE)== 0) {
                    PrintBinOp(prx->Beg, prx->End, prx->Level - 3, prx->DistanceType, n);
                    Request << SPACE_STR;
                }
            }
        }
        //пишем последнего ребенка
        if (printMetaData)
            PrintBeginOfSynonymGroup(n, pos);
        PrintRichRequest((*end).Get(), qChildNearPhrase, ft, n->Necessity, nullptr);
        if (printMetaData)
            PrintEndOfSynonymGroup(n, pos);

        PrintQuota(n);

        if (NeedParen(n) && !PrintingZoneAttr)
            Request << RIGHT_PAREN_STR;

        if ((oRestrDoc != n->Op()) && (oRefine != n->Op()) && (oAndNot != n->Op()))
            if (NeedParen(n) && PrintingZoneAttr)
                Request << RIGHT_PAREN_STR;

        qNearPhrase = qChildNearPhrase;
    }


    void TRichRequestCollector::PrintZone(const TRichRequestNode* n, TNodeNecessity& parN,
                                          const TRichRequestNode* parent, bool start){
        Y_ASSERT(parent);
        if (start){
            PrintNecessity(n, parN);
            Request << n->GetTextName();
            Y_ASSERT(n->Children.size() <= 1);
            if (n->OpInfo.Arrange) {
                Request << CMP_RIGHT_ARROW << LEFT_PAREN_STR;
            }

            if (n->Children.size() == 1) {
                if (!!n->GetTextName() && !n->OpInfo.Arrange) {
                    //[zone:(]
                    Request << NEW_CMP_EQ_STR << LEFT_PAREN_STR;
                }
                PrintingZone = true;                    //начали писать oZone
                PrintRichRequest(n->Children[0].Get(), 0, fGeneral, n->Necessity, n, start);
                Y_ASSERT(!PrintingZone);
            }
            if (PrintingZoneAttr) {                 //[zone:(attr:(value]
                Request << SPACE_STR;
                PrintingZoneAttr = false;           //закончили писать значение атрибута для oZone
            } else if (!parent->Children.empty() || !!parent->GetText()) {
                if (!n->OpInfo.Arrange) {
                    Request << NEW_CMP_EQ_STR << LEFT_PAREN_STR;
                }
            }

        } else {
            if (!parent->Children.empty() || !!parent->GetText()){
                Request << RIGHT_PAREN_STR;
            }
        }
    }

    void TRichRequestCollector::PrintRestrictByPos(const TRichRequestNode* n, bool start){
        if (!start){
            PrintRestrictBounds(n->OpInfo);
        }
    }

    void TRichRequestCollector::PrintAttr(const TRichRequestNode* n, TMaybe<TFormType> parFT, TNodeNecessity parN){
        PrintNecessity(n, parN);
        if (!PrintingZoneAttr) {
            Request << n->GetTextName();           //[attr]
            PrintCmpOp(n);                     //[attr:]
        }
        const TUtf16String& wattr = n->GetText();
        if (!!wattr) {
            if (IsQuoteChar(wattr[0])) {
                Request << wattr << wattr[0];  //[attr="value"]
            } else {
                PrintFormType(n, parFT);
                Request << wattr;        //[attr:value] - atrr -numeric
            }
            PrintReverseFreq(n);

            if (PrintingZone) {
                PrintingZoneAttr = true;    //начали печатать значение атрибута.
            }
            PrintingZone = false;
        }
    }

    void TRichRequestCollector::PrintAttrInterval(const TRichRequestNode* n, TNodeNecessity parN){
        PrintNecessity(n, parN);
        const TUtf16String& name = n->GetTextName();
        const TUtf16String& wattr = n->GetText();
        const TUtf16String& wattrhi = n->GetAttrValueHi();

        Request << name << NEW_CMP_EQ_STR << wattr << NEW_DOUBLE_DOT;
        if (IsQuoteChar(wattr.at(0)) && IsQuoteChar(wattrhi.at(0))) {
            Request << TWtringBuf(wattrhi).Skip(1) << wattrhi[0];
        } else {
            Request << wattrhi;
        }
    }

    void TRichRequestCollector::PrintRichRequest(const TRichRequestNode* n,
                                                 int qNearPhrase, TMaybe<TFormType> parFT,
                                                 TNodeNecessity parN, const TRichRequestNode* parent,
                                                 bool start)
    {
        Y_ASSERT(n);
        TOperType op = n->Op();
        const bool printMetaData = !(Options & PRRO_IgnoreMetaData);
        const bool printMiscOps = printMetaData && !(IsWordOrMultitoken(*n) && (Options & PRRO_WordNoMiscOps));
        if (printMiscOps)
            PrintPreMiscOps(n, qNearPhrase, parFT, parN);
        if (IsBinOp(n))
            PrintBinOp(n, qNearPhrase, parFT, parN, printMetaData);
        else {
            if (printMetaData && IsAttribute(*n)) {
                if (!n->GetAttrValueHi())
                    PrintAttr(n, parFT, parN);
                else
                    PrintAttrInterval(n, parN);
            } else if (IsWordOrMultitoken(*n) && !n->Children.size()) {
                if (printMetaData)
                    PrintBeginOfSynonymGroup(n, 0);
                PrintWord(n, parFT, parN, printMetaData);
                if (printMetaData)
                    PrintEndOfSynonymGroup(n, 0);
            } else if (printMetaData) {
                switch (op) {
                case oZone:
                    PrintZone(n, parN, parent, start);
                    break;
                case oRestrictByPos:
                    PrintRestrictByPos(n, start);
                    break;
                default:
                    break;
                }
            }
        }
        if (printMiscOps)
            PrintPostMiscOps(n, qNearPhrase, parFT, parN);
    }

    void TRichRequestCollector::PrintPreMiscOps(const TRichRequestNode* n, int qNearPhrase,
                                                TMaybe<TFormType>& parFT, TNodeNecessity& parN){
        TVector<TRichNodePtr>::const_iterator it, end;
        end = n->MiscOps.end();
        for (it = n->MiscOps.begin(); it != end; ++it)
            if ((oRestrDoc != (*it)->Op()) && (oAndNot != (*it)->Op()) && (oRefine != (*it)->Op()))
                PrintRichRequest((*it).Get(), qNearPhrase, parFT, parN, n, true);
        else
            PrintPreRestrDoc((*it).Get(), parFT, parN);
    }

    void TRichRequestCollector::PrintPostMiscOps(const TRichRequestNode* n, int qNearPhrase,
                                                 TMaybe<TFormType> parFT, TNodeNecessity parN){
        TVector<TRichNodePtr>::const_iterator it, begin;
        begin = n->MiscOps.begin();
        if (!n->MiscOps.empty()){
            it = n->MiscOps.end();
            do {
                --it;
                PrintRichRequest((*it).Get(), qNearPhrase, parFT, parN, n, false);
                if (!((oRestrDoc != (*it)->Op()) && (oAndNot != (*it)->Op()) && (oRefine != (*it)->Op())))
                    PrintPostRestrDoc((*it).Get());
            } while (it != begin);
        }
    }

} //NPrivate

TUtf16String PrintRichRequest(const NSearchQuery::TRequest* n, EPrintRichRequestOptions options) {
    NPrivate::TRichRequestCollector printer(n->Root.Get(), n->Softness, options);
    return printer.PrintRichRequest();
}

TUtf16String PrintRichRequest(const TRichRequestNode& n, EPrintRichRequestOptions options) {
    NPrivate::TRichRequestCollector printer(&n, 0, options);
    return printer.PrintRichRequest();
}

//------------------------------------------------------------------------------------------------------
//Отладочная печать дерева
IOutputStream& operator <<(IOutputStream& out, const TOperType& operType);
IOutputStream& operator <<(IOutputStream& out, const TPhraseType& phraseType);
IOutputStream& operator <<(IOutputStream& out, const TFormType&);
IOutputStream& operator <<(IOutputStream& out,  const TCompareOper&);
IOutputStream& operator <<(IOutputStream& out,  const TNodeNecessity&);

static inline TString ToOp(const TRichRequestNode* n) {
    if (IsWord(*n))
        return "oWord";
    else if (IsAttribute(*n))
       return "oAttr";
    return ToString(n->OpInfo.Op);
}

static void PrintRichTree(const TRichRequestNode* n, IOutputStream& out, const TString& indent, ui32 s) {
    Y_ASSERT(n);
    TString name = WideToUTF8(n->GetTextName());
    TString text = WideToUTF8(n->GetText());
    TString hi = WideToUTF8(n->GetAttrValueHi());
    TString factor = WideToUTF8(n->GetFactorName());

    out << indent << "Name: " << name << ", Text: " << text;
    if (!!hi)
        out << ", Hi: " << hi;
    if (!!factor)
        out << ", Factor: " << factor << ", FactorValue: " << n->GetFactorValue();

    out << ", Operation: { " << n->OpInfo.Lo << ", " << n->OpInfo.Hi << ", " << ToOp(n)
        << ", " << n->OpInfo.Level << ", " << n->OpInfo.CmpOper << ", " << n->OpInfo.Arrange << "}"
        << ", N:" << n->Necessity << ", "
        << (n->Parens ? "sePARENS" : "seNONE") << ", " << n->GetPhraseType();
    if (n->IsQuoted())
        out << ", Quoted";
    out << ", RF:"   << n->ReverseFreq << ", S:" << s;

    TWordNode* wi = n->WordInfo.Get();
    if (wi) {
        NLP_TYPE t = wi->IsLemmerWord()?NLP_WORD:(wi->GetLemmas().size() == 1?GuessTypeByWord(wi->GetLemmas()[0].GetLemma().data(),wi->GetLemmas()[0].GetLemma().size()):NLP_END);
        out << "; WordInfo: (NLP_TYPE, " << t
            << "), (FormType, " << wi->GetFormType()
            << "), (IsStopWord, " << wi->IsStopWord() << ") " << Endl;
    } else
        out << Endl;

    TVector<TRichNodePtr>::const_iterator it1, end;
    TString next_indent = indent + "    ";

    if (!n->MiscOps.empty()){
        out << indent << "MiscOps:" << Endl;
        end = n->MiscOps.end();
        for (it1 = n->MiscOps.begin(); it1 != end; ++it1)
            PrintRichTree((*it1).Get(), out, next_indent, 0);
    }

    if (!n->Children.empty()){
        out << indent << "Children:" << Endl;
        end = n->Children.end();
        for (it1 = n->Children.begin(); it1 != end; ++it1)
            PrintRichTree((*it1).Get(), out, next_indent, 0);
    }

    {
        bool first = true;
        for (NSearchQuery::TForwardMarkupIterator<TSynonym, true> it2(n->Markup()); !it2.AtEnd(); ++it2) {
            if (first) {
                out << indent << "Synonyms:" << Endl;
                first = false;
            }
            PrintRichTree(it2.GetData().SubTree.Get(), out, next_indent, 0);
        }
    }
}

void PrintRichTree(const NSearchQuery::TRequest* r, IOutputStream& out) {
    PrintRichTree(r->Root.Get(), out, "", r->Softness);
}

typedef THashMap<const TRichRequestNode*, TUtf16String> TNodeNameHash;

static void CollectNodeNames(const TRichRequestNode* n, TNodeNameHash& namehash);

static void CollectNodeNamesWithSurroundingText(const TRichRequestNode* n, TNodeNameHash& namehash, TUtf16String& nodeText, TUtf16String& textBefore, TUtf16String& textAfter) {
    if (n->Children.empty()) {
        nodeText = n->GetText();
        textBefore = n->GetTextBefore();
        textAfter = n->GetTextAfter();
    } else {
        for (size_t i = 0; i < n->Children.size(); ++i) {
            if (i == 0) {
                CollectNodeNamesWithSurroundingText(n->Children[i].Get(), namehash, nodeText, textBefore, textAfter);
            } else {
                TUtf16String childText;
                TUtf16String childTextBefore;
                CollectNodeNamesWithSurroundingText(n->Children[i].Get(), namehash, childText, childTextBefore, textAfter);
                nodeText += childTextBefore;
                nodeText += childText;
            }
        }
    }

    for (size_t i = 0; i < n->MiscOps.size(); ++i) {
        CollectNodeNames(n->MiscOps[i].Get(), namehash);
    }

    for (NSearchQuery::TForwardMarkupIterator<TSynonym, true> it(n->Markup()); !it.AtEnd(); ++it) {
        CollectNodeNames(it.GetData().SubTree.Get(), namehash);
    }
    namehash[n] = nodeText;
}

static void CollectNodeNames(const TRichRequestNode* n, TNodeNameHash& namehash) {
    TUtf16String text;
    TUtf16String textBefore;
    TUtf16String textAfter;

    CollectNodeNamesWithSurroundingText(n, namehash, text, textBefore, textAfter);
}

static void PrintRichNodeAsText(const TRichRequestNode* n, IOutputStream& out, const TNodeNameHash& namehash) {
    TNodeNameHash::const_iterator nameit = namehash.find(n);
    if (nameit == namehash.end())
        out << "<uncashed>";
    else
        out << nameit->second;
}


static void PrintBasicWordInfo(const TWordNode* w, IOutputStream& out) {
    if (!w)
        return;

    out << " <";
    const TWordInstance::TLemmasVector& lemmas = w->GetLemmas();
    for (TWordInstance::TLemmasVector::const_iterator it = lemmas.begin();
         it != lemmas.end(); ++it) {
        if (it != lemmas.begin())
            out << ", ";
        out << it->GetLemma() << " [" << ::NameByLanguage(it->GetLanguage());
        if (!it->GetStemGrammar().Empty()) {
            out << " " << it->GetStemGrammar().ToString(",", true);
        }
        if (it->GetQuality()) {
            if ( (TYandexLemma::QBastard & it->GetQuality()) )
                out << "; bastard";
            else if ( (TYandexLemma::QSob & it->GetQuality()) )
                out << "; fastdict";
            else if ( (TYandexLemma::QFoundling & it->GetQuality()) )
                out << "; foundling";
            if ( (TYandexLemma::QPrefixoid & it->GetQuality()) )
                out << ", prefixoid";
            if ( (TYandexLemma::QBadRequest & it->GetQuality()) )
                out << "; badrequest";
        }
        if (it->IsStopWord())
            out << "; stopword";
        out << "]";
    }
    out << ">";
}

static void PrintBasicDistance(const TProximity& p, const char* op, IOutputStream& out) {
    out << " " << op;
    if (p.Level == DOC_LEVEL)
        out << op;
    if (p.Beg != -32768 || p.End != 32768) {
        out << "/(" << p.Beg << "," << p.End << ")";
    }
    switch (p.DistanceType) {
        case DT_NONE:
            out << "<none>";
            break;
        case DT_MULTITOKEN:
            out << "M";
            break;
        case DT_PHRASE:
            break;
        case DT_QUOTE:
            out << "Q";
            break;
        case DT_USEROP:
            out << "U";
            break;
        case DT_SYNTAX:
            out << "S";
            break;
        default:
            out << "<unknown DT>";
            break;
    }
    out << " ";
}

static void PrintBasicRichTree(const TRichRequestNode* n, IOutputStream& out, const TString& indent, const TNodeNameHash& namehash, bool skip_initial_indent = false) {
    if (!skip_initial_indent)
        out << indent;
    if (n->Children.size() == 1) {
        if (n->Op() == oRefine)
            out << "<- ";
        else if (n->Op() == oRestrDoc)
            out << "<< ";
    }

    if (n->GetFormType() == fExactWord)
        out << "! ";
    else if (n->GetFormType() == fExactLemma)
        out << "!! ";

    PrintRichNodeAsText(n, out, namehash);

    PrintBasicWordInfo(n->WordInfo.Get(), out);
    if (n->ReverseFreq != -1)
        out << " :: " << n->ReverseFreq;
    out << Endl;

    TString next_indent = indent + "    ";
    TString next_next_indent = next_indent + "    ";
    if (!n->Children.empty()) {
        if (n->Children.size() > 1) {
            out << indent << "  = ";
            for (size_t i = 0; i < n->Children.size(); ++i) {
                if (i) {
                    switch (n->Op()) {
                        case oAnd:
                            PrintBasicDistance(n->Children.ProxBefore(i), "&", out);
                            break;
                        case oAndNot:
                            PrintBasicDistance(n->Children.ProxBefore(i), "~", out);
                            break;
                        case oOr:
                            out << "|" << Endl;
                            break;
                        case oWeakOr:
                            out << "^" << Endl;
                            break;
                        default:
                            out << "<unsupported n-ary operator>" << Endl;
                            break;
                    }
                }
                out << "(";
                PrintRichNodeAsText(n->Children[i].Get(), out, namehash);
                out << ")";
            }
            out << " =" << Endl;
        }

        for (size_t i = 0; i < n->Children.size(); ++i) {
            PrintBasicRichTree(n->Children[i].Get(), out, next_indent, namehash);
        }
    }

    // if (!n->MiscOps.empty()) {
    //     out << Endl << next_indent << "========== Misc Ops ==========" << Endl;
    //     for (size_t i = 0; i < n->MiscOps.size(); ++i)
    //         PrintBasicRichTree(n->MiscOps[i].Get(), out, next_indent, namehash);
    // }

    if (!n->Markup().Empty()) {
        out << next_indent << "========== Synonyms ==========" << Endl;

        for (NSearchQuery::TForwardMarkupIterator<TSynonym, true> it(n->Markup()); !it.AtEnd(); ++it) {
            out << next_indent;
            if (n->Children.empty()) {
                PrintRichNodeAsText(n, out, namehash);
            } else {
                for (size_t i = it->Range.Beg; i <= it->Range.End; ++i) {
                    if (i >= n->Children.size()) {
                        if (i != it->Range.Beg)
                            out << " ";
                        out << "WORD#" << i;
                    } else {
                        if (i != it->Range.Beg)
                            out << n->Children[i].Get()->GetTextBefore();
                        PrintRichNodeAsText(n->Children[i].Get(), out, namehash);
                    }
                }
            }
            out << " ^ ";
            PrintBasicRichTree(it.GetData().SubTree.Get(), out, next_next_indent, namehash, true);
        }
    }
}

void PrintBasicRichTree(const NSearchQuery::TRequest* r, IOutputStream& out) {
    // out << "Softness: " << r->Softness << Endl;
    TNodeNameHash namehash;
    CollectNodeNames(r->Root.Get(), namehash);
    PrintBasicRichTree(r->Root.Get(), out, "", namehash);
}


template <bool normalized>
static inline TUtf16String ExtractNodeText(const TRichRequestNode& node) {
    if (normalized && node.WordInfo.Get() != nullptr)
        return node.WordInfo->GetNormalizedForm();
    else
        return node.GetText();
}

template <bool normalized>
static TUtf16String CollectChildrenText(const TRichRequestNode& node) {
    if (node.Children.empty())
        return TUtf16String();

    TUtf16String res = ExtractNodeText<normalized>(*node.Children[0]);

    for (size_t i = 1; i < node.Children.size(); ++i) {
        res += node.Children[i-1]->GetPunctAfter();
        res += ExtractNodeText<normalized>(*node.Children[i]);
    }

    return res;
}

template <bool normalized>
static inline TUtf16String GetRichRequestNodeTextImpl(const TRichRequestNode& node) {
    TUtf16String res = ExtractNodeText<normalized>(node);
    if (!res && node.GetPhraseType() == PHRASE_MULTIWORD)
        res = CollectChildrenText<normalized>(node);
    return res;
}

TUtf16String GetRichRequestNodeText(const TRichRequestNode* node) {
    return GetRichRequestNodeTextImpl<true>(*node);
}

TUtf16String GetOriginalRichRequestNodeText(const TRichRequestNode* node) {
    return GetRichRequestNodeTextImpl<false>(*node);    // not normalized
}

void DebugPrintRichTree(const TRichRequestNode& node, IOutputStream& out) {
    out << '"' << node.GetText() << '"';
    if (!node.Children.empty()) {
        out << ":[";
        DebugPrintRichTree(*node.Children[0], out);
        for (size_t i = 1; i < node.Children.size(); ++i) {
            out << "{" << node.Children[i]->GetTextBefore() << "}";
            DebugPrintRichTree(*node.Children[i], out);
        }
        out << "]";
    }
}

TString DebugPrintRichTree(const TRichRequestNode& node) {
    TStringStream str;
    DebugPrintRichTree(node, str);
    return str.Str();
}

void PrintInfo(const TRichRequestNode& node, ui32 level, ui32 syn) {
    if (level == 0) {
        Cerr << "----------------------------------- PrintInfo" << Endl;
    }
    TString s(level, '-');
    Cerr << s << node.GetText() << "/" << node.GetTextName() << "/" << node.Op() << Endl;
    for (size_t i = 0; i < node.Children.size(); ++i) {
        PrintInfo(*node.Children[i], level + 1);
    }

    NSearchQuery::TForwardMarkupIterator<TSynonym, true> i(node.Markup());
    if (!i.AtEnd()) {
        if (!syn)
            Cerr << s << "SYNONYMS:" << Endl;
        for (; !i.AtEnd(); ++i) {
            PrintInfo(*(i.GetData()).SubTree, level + 1, syn + 1);
        }
        if (!syn)
            Cerr << s << "SYNONYMS FINISHED;" << Endl;
    }
}
