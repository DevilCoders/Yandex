#include <util/string/util.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/generic/string.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <library/cpp/charset/codepage.h>
#include <util/charset/wide.h>
#include <util/charset/unidata.h>
#include <library/cpp/uri/http_url.h>
#include <ysite/yandex/common/prepattr.h>

#include "reqscan.h"
#include "request.h"
#include "req_pars.h"
#include "simpleparser.h"
#include "reqattrlist.h"

const char ATTR_PREVREQ_STR[] = "prevreq";
const char ATTR_SOFTNESS_STR[] = "softness";
const char ATTR_INPOS_STR[] = "inpos";
const char ATTR_REFINE_FACTOR_STR[] = "refinefactor";

namespace {
    using NTokenClassification::TTokenTypes;
    using NTokenClassification::IsClassifiedToken;

    typedef std::pair<size_t, size_t> TPosLen;

    const size_t DATE_VALUE_LEN = 8;        // "20081223"
    const size_t DATETIME_VALUE_LEN = 19;     // "2008-12-23T18:39:21"
    const size_t DATETIME_NO_SECONDS_VALUE_LEN = 16;     // "2008-12-23T18:39"
    const size_t DATETIME_NO_MINUTES_VALUE_LEN = 13;     // "2008-12-23T18"
    const size_t TWODOTS_LEN = 2;
    const size_t TWOAPOSTROPHES_TWODOTS_LEN = 4;

    const wchar16 APOSTROPHE_CHAR = '\'';

    const wchar16 TWODOTS_STR[] = { '.', '.', 0 };
    const wchar16 TWOAPOSTROPHES_TWODOTS_STR[] = { APOSTROPHE_CHAR, '.', '.', APOSTROPHE_CHAR, 0 };
    const wchar16 ONE_STR[] = { '1', 0 };
    const wchar16 ZERO_STR[] = { '0', 0 };
    const wchar16 FALSE_STR[] = { 'f', 'a', 'l', 's', 'e', 0 };
    const wchar16 TRUE_STR[] = { 't', 'r', 'u', 'e', 0 };
    const wchar16 ON_STR[] = { 'o', 'n', 0 };
    const wchar16 OFF_STR[] = { 'o', 'f', 'f', 0 };
    const wchar16 YES_STR[] = { 'y', 'e', 's', 0 };
    const wchar16 NO_STR[] = { 'n', 'o', 0 };

    // no 0 at the end of the buffer
    const wchar16 JAN_01[] = { '0', '1', '0', '1' };
    const wchar16 DEC_31[] = { '1', '2', '3', '1' };
    const wchar16 DAY_01[] = { '0', '1' };
    const wchar16 DAY_31[] = { '3', '1' };

    const wchar16 QUOTATION_CHAR = '"';
    const wchar16 ASTERISK_CHAR = '*';

    //! returns pair - position and length of the value after scanning a name:value entry
    //! @param entry    information prepared by the scanner, @c EntryPos
    //! @param toklen   length of the whole token
    TPosLen GetAttrValuePosLen(const TLangEntry& e, const wchar16* tokend) {
        //   +--+ entry.EntryLeng
        //          +---------+ valueLen (with quotes)
        // +!name:<="attrvalue"
        //   ^      ^
        //   |      pos (start position of value)
        //   entry.EntryPos
        // OR
        // +!name->"attrvalue"
        //   ^     ^
        //   |     pos (start position of value)
        //   entry.EntryPos

        const size_t n = e.EntryLeng + (IsRankingSearchInsideZone(e) ? 2 : 1 + GetCmpOperLen(e.OpInfo.CmpOper));
        // 2 - length of "->", 1 - length of ':'

        // position of the value in entry.Text:
        const size_t pos = e.EntryPos + n;

        Y_ASSERT(tokend && tokend > e.Text + pos && tokend <= e.Text + std::char_traits<wchar16>::length(e.Text));

        // length of the value with quotes:
        const size_t len = tokend - (e.Text + pos);

        return std::make_pair(pos, len);
    }

    //! returns @c false if value is not converted to int or if int is not in the range 0..100
    inline bool GetSoftnessAttrValue(const TUtf16String& str, ui32& val) {
        Y_ASSERT(!str.empty());

        try {
            val = FromString<ui32>(str);
        } catch (const std::exception& /*e*/) {
            return false;
        }
        ui32 minVal = SOFTNESS_MIN; // to eliminate warning "comparison of unsigned expression >= 0 is always true"
        return (val >= minVal && val <= SOFTNESS_MAX);
    }

    //! returns @c true if all characters are within ranges A-Z, a-z, 0-9
    inline bool CheckPrevreqAttrValue(const TUtf16String& value) {
        Y_ASSERT(!value.empty());
        const wchar16* first = value.c_str();
        const wchar16* const last = first + value.size();
        for (; first != last; ++first) {
            if (*first > 127 || !isalnum(*first))
                return false;
        }
        return true;
    }

    TRequestNode* ParseText(const TReqTokenizer& parentTokenizer, const wchar16* text, size_t offset, size_t length) {
        Y_ASSERT(text && length);
        // in order to keep position numbers of words as in the initial text
        // the text needs to be cut:
        // one cat:(abc 123) two -> one cat:(abc 123)
        //                  ^               ^
        //                  cut here        start will be here
        const TUtf16String s(text, offset + length);
        Y_ASSERT(offset + length <= s.size());

        THolder<TReqTokenizer> tokenizer(parentTokenizer.Clone(s, parentTokenizer.request, &parentTokenizer.GetNodeFactory(), offset));

        if (ParseYandexRequest(tokenizer.Get())) {
            return nullptr;
        }
        if (!tokenizer->Error.empty() && (parentTokenizer.request.GetProcessingFlag() & RPF_CLEAN_REQUESTS_ONLY)) {
            return nullptr;
        }

        return tokenizer->GetResult();
    }

    inline TRequestNode* ConvertAttrNodeToWordNodes(const TReqTokenizer& tokenizer, const TLangEntry& entry, const TPosLen& pl) {
        TRequestNode::TFactory& factory = tokenizer.GetNodeFactory();
        TRequestNode* left(factory.CreateWordNode(entry, TWideToken(entry.Text + entry.EntryPos, entry.EntryLeng)));
        Y_ASSERT(left);
        TRequestNode* right(ParseText(tokenizer, entry.Text, pl.first, pl.second));
        if (!right) // in case of syntax error
            return left;

        TLangSuffix ls = DefaultLangSuffix;
        ls.OpInfo.Op = oAnd;
        // @todo probably prefix should be taken from the node?
        return factory.CreateBinaryOper(left, right, DefaultLangPrefix, ls, PHRASE_PHRASE);
    }

    inline bool CreateInposAttrNode(const TUtf16String& attrValue, TLangSuffix& suffix) {
        ui32 loval = 0;
        ui32 hival = 0;

        try {
            size_t pos = attrValue.find(TWODOTS_STR);
            if (pos == TUtf16String::npos)
                return false;
            loval = FromString<ui32>(attrValue.substr(0, pos));
            hival = FromString<ui32>(attrValue.c_str() + pos + TWODOTS_LEN);
        } catch (...) {
            return false;
        }

        suffix.OpInfo.Op = oRestrictByPos;
        suffix.OpInfo.Lo = static_cast<i32>(loval);
        suffix.OpInfo.Hi = static_cast<i32>(hival);
        return true;
    }

    //! returns @c true if value is converted to unsigned integer
    //! @note at the present time it checks integer attributes only
    inline bool CheckIntegerAttributeValue(const TUtf16String& value) {
        try {
            FromString<ui32>(value);
        } catch (...) {
            return false;
        }
        return true;
    }

    inline bool AreDigits(const wchar16* first, const wchar16* last) {
        for (; first != last; ++first) {
            if (!isdigit(*first))
                return false;
        }
        return true;
    }

    inline bool CheckDateAttributeValue(const TUtf16String& value) {
        switch (value.size())
        {
        case 5:    // date:2008*
        case 7:    // date:200812*
            return AreDigits(value.begin(), value.end() - 1) && value.back() == ASTERISK_CHAR;

        case 4:    // date:2008
        case 6:    // date:200812
        case DATE_VALUE_LEN:    // date:20081222
            return AreDigits(value.begin(), value.end());

        default:
            Y_ASSERT(false);
            return false;
        }
    }

    //! extracts from text date year, month, day and seconds from the beginning of the day
    //! @param value        text date, year, month and day are returned in this parameter
    //! @param seconds      seconds from the beginning of the day
    //! @return @c false if format of the date is invalid, the only supported format is "yyyy-mm-ddThh:mm:ss"
    inline bool GetDateTimeAttrValue(TUtf16String& value, ui32& seconds) {
        Y_ASSERT(value.size() == DATETIME_VALUE_LEN);    // date:2008-12-22T18:29:11
        try {
            wchar16 buf[10];
            ui32 hour = 0, min = 0, sec = 0;
            TSimpleParser<wchar16>(value.c_str())
                .Copy(4, buf, '0', '9')
                .Check('-')
                .Copy(2, buf + 4, '0', '9')
                .Check('-')
                .Copy(2, buf + 6, '0', '9')
                .Check('T')
                .Parse(2, hour)
                .Check(':')
                .Parse(2, min)
                .Check(':')
                .Parse(2, sec);
            buf[8] = 0;
            value.assign(buf, 8);
            seconds = hour * 3600 + min * 60 + sec;
            return true;
        } catch (...) {
            return false;
        }
    }

    inline bool GetDateTimeNoSecondsAttrValue(TUtf16String& value, ui32& seconds) {
        Y_ASSERT(value.size() == DATETIME_NO_SECONDS_VALUE_LEN);    // date:2008-12-22T18:29
        try {
            wchar16 buf[10];
            ui32 hour = 0, min = 0;
            TSimpleParser<wchar16>(value.c_str())
                .Copy(4, buf, '0', '9')
                .Check('-')
                .Copy(2, buf + 4, '0', '9')
                .Check('-')
                .Copy(2, buf + 6, '0', '9')
                .Check('T')
                .Parse(2, hour)
                .Check(':')
                .Parse(2, min);
            buf[8] = 0;
            value.assign(buf, 8);
            seconds = hour * 3600 + min * 60;
            return true;
        } catch (...) {
            return false;
        }
    }

    inline bool GetDateTimeNoMinutesAttrValue(TUtf16String& value, ui32& seconds) {
        Y_ASSERT(value.size() == DATETIME_NO_MINUTES_VALUE_LEN);    // date:2008-12-22T18
        try {
            wchar16 buf[10];
            ui32 hour = 0;
            TSimpleParser<wchar16>(value.c_str())
                .Copy(4, buf, '0', '9')
                .Check('-')
                .Copy(2, buf + 4, '0', '9')
                .Check('-')
                .Copy(2, buf + 6, '0', '9')
                .Check('T')
                .Parse(2, hour);
            buf[8] = 0;
            value.assign(buf, 8);
            seconds = hour * 3600;
            return true;
        } catch (...) {
            return false;
        }
    }

    inline bool SplitInvervalValueImpl(TWtringBuf value, TUtf16String& loval, TUtf16String& hival, const wchar16* delimiter, size_t delimiterLen) {
        size_t pos = value.find(delimiter);
        if (pos == TUtf16String::npos)
            return false;
        loval = value.substr(0, pos);
        hival = value.substr(pos + delimiterLen);
        return true;
    }

    //! splits text value into two values that were delimited by two dots
    //! @return @c false if two dots not found
    inline bool SplitInvervalValue(const TUtf16String& value, TUtf16String& loval, TUtf16String& hival) {
        return SplitInvervalValueImpl(value, loval, hival, TWODOTS_STR, TWODOTS_LEN);
    }

    //! splits text value into two values that were delimited by two dots and in quots
    //! @return @c false if two dots not found
    inline bool SplitLiteralInvervalValue(const TUtf16String& value, TUtf16String& loval, TUtf16String& hival) {
        if (value.length() < TWOAPOSTROPHES_TWODOTS_LEN + 3) //"''..''
            return false;
        TWtringBuf clearValue = TWtringBuf(value.data() + 1, value.size() - 1);
        if (clearValue.EndsWith(QUOTATION_CHAR)) {
            clearValue.Chop(1);
        }

        if (!clearValue.StartsWith(APOSTROPHE_CHAR) || !clearValue.EndsWith(APOSTROPHE_CHAR)) {
            return false;
        }
        if (!SplitInvervalValueImpl(clearValue.substr(1, clearValue.length() - 2), loval, hival, TWOAPOSTROPHES_TWODOTS_STR, TWOAPOSTROPHES_TWODOTS_LEN)) {
            return false;
        }
        hival.insert(size_t(0), 1, QUOTATION_CHAR);
        loval.insert(size_t(0), 1, QUOTATION_CHAR);
        return true;
    }

    inline bool GetBooleanAttributeValue(TUtf16String& value) {
        TUtf16String w(value);
        w.to_lower();

        // @todo add yes/no in russian
        if (w == ONE_STR || w == TRUE_STR || w == YES_STR || w == ON_STR) {
            value = ONE_STR;
            return true;
        }
        if (w == ZERO_STR || w == FALSE_STR || w == NO_STR || w == OFF_STR) {
            value = ZERO_STR;
            return true;
        }
        return false;
    }

    inline void SetAttributeNode(TRequestNode& node, const TPosLen& pl, const TUtf16String& name, const TUtf16String& value, const TUtf16String& hival) {
        node.FormType = fGeneral; // reset form type if it is not fGeneral
        node.Span = TCharSpan(pl.first, pl.second); // position and length should be changed to the position and length of the value
        node.SetAttrValues(name, value, hival);
    }

    inline int CreateSpecialAttrNode(TReqTokenizer& tokenizer, const wchar16* tokend, YYSTYPE& lval) {
        TUtf16String name(GetNameByEntry(tokenizer.GetEntry()));
        Y_ASSERT(tokenizer.request.GetAttrType(name) == RA_ATTR_SPECIAL);

        const TLangEntry& entry = tokenizer.GetEntry();
        Y_ASSERT(tokend >= entry.Entry());

        const TPosLen pl = GetAttrValuePosLen(entry, tokend);
        TUtf16String attrValue(entry.Text + pl.first, pl.second);

        if (name == SOFTNESS_STR) {
            ui32 value = 0;
            if (GetSoftnessAttrValue(attrValue, value)) {
                tokenizer.SetSoftness(value);
                lval.pnode = nullptr;
                return ATTR_VALUE;
            }
        } else if (name == NGR_SOFTNESS_STR) {
            try {
                TVector<TStringBuf> ngrSoftnessInfo;
                TString ngrInfoStr = WideToUTF8(attrValue);
                StringSplitter(ngrInfoStr).Split(',').SkipEmpty().Collect(&ngrSoftnessInfo);
                for (ui32 i = 0; i < ngrSoftnessInfo.size(); ++i) {
                    TVector<TStringBuf> ngrZoneInfo;
                    StringSplitter(ngrSoftnessInfo[i]).Split('-').SkipEmpty().Collect(&ngrZoneInfo);
                    ui32 softInfo;
                    if (ngrZoneInfo.size() == 2 && TryFromString(ngrZoneInfo[1], softInfo)) {
                        tokenizer.AddNgrSoftnessInfo(TString(ngrZoneInfo[0]), softInfo);
                    }
                }
                lval.pnode = nullptr;
                return ATTR_VALUE;
            } catch (const std::exception& /*e*/) {}
        } else if (name == PREFIX_STR) {
            try {
                TVector<TStringBuf> prefixes;
                TString prefStr = WideToUTF8(attrValue);
                StringSplitter(prefStr).Split(',').SkipEmpty().Collect(&prefixes);
                for (ui32 i = 0; i < prefixes.size(); ++i)
                    tokenizer.AddKeyPrefix(FromString<ui64>(prefixes[i]));
                lval.pnode = nullptr;
                return ATTR_VALUE;
            } catch (const std::exception& /*e*/) {}
        } else if (name == PREVREQ_STR) {
            if (CheckPrevreqAttrValue(attrValue)) {
                const TLangEntry& le = tokenizer.GetEntry();
                lval.pnode = tokenizer.GetNodeFactory().CreateAttrTypeNode(le);
                // Adjust the span of the topmost node
                lval.pnode->Span.Len = tokend - entry.Entry();
                lval.pnode->FullSpan.Len = tokend - entry.Entry();

                attrValue.insert(size_t(0), 1, QUOTATION_CHAR);
                SetAttributeNode(*lval.pnode, pl, name, attrValue, TUtf16String());
                return ATTR_VALUE;
            }
        } else if (name == INPOS_STR) {
            TLangSuffix suffix = DefaultLangSuffix;
            if (CreateInposAttrNode(attrValue, suffix)) {
                lval.suffix = suffix;
                return RESTRBYPOS;
            }
        } else if (name == REFINE_FACTOR_STR) {
            lval.pnode = tokenizer.GetNodeFactory().CreateAttrTypeNode(entry);
            // Adjust the span of the topmost node
            lval.pnode->Span.Len = tokend - entry.Entry();
            lval.pnode->FullSpan.Len = tokend - entry.Entry();
            SetAttributeNode(*lval.pnode, pl, name, attrValue, TUtf16String());
            return ATTR_VALUE;
        }

        lval.pnode = ConvertAttrNodeToWordNodes(tokenizer, entry, pl);
        // Adjust the span of the topmost node
        lval.pnode->Span.Len = tokend - entry.Entry();
        lval.pnode->FullSpan.Len = tokend - entry.Entry();
        return ATTR_VALUE;
    }

    inline void CreateLiteralAttributeNode(TRequestNode& node, bool quoted, const TPosLen& pl, const TUtf16String& name, const TUtf16String& attrValue) {
        TUtf16String value(attrValue);
        // attribute must always have double quote before text value
        if (quoted)
            value.replace(0, 1, 1, QUOTATION_CHAR); // always replace the first char with "
        else
            value.insert(size_t(0), 1, QUOTATION_CHAR); // insert " to the first position of the value
        TUtf16String loVal, hiVal;
        if (SplitLiteralInvervalValue(value, loVal, hiVal)) {
            SetAttributeNode(node, pl, name, loVal, hiVal);
        } else {
            SetAttributeNode(node, pl, name, value, TUtf16String());
        }
    }

    inline void CreateLiteralUrlAttributeNode(TRequestNode& node, bool quoted, const TPosLen& pl, const TUtf16String& name, const TUtf16String& attrValue) {
        TUtf16String value(attrValue);

        if (quoted)
            value.erase(0, 1); // remove " (or any other char meaning ")

        Strip(value);
        value = CutHttpPrefix(value);

        value.insert(size_t(0), 1, QUOTATION_CHAR); // insert " to the first position of the value

        SetAttributeNode(node, pl, name, value, TUtf16String());
    }

    //! @note returns true if asterisk is the last character and there is no slash before it
    //!       see also LoadAttr() in ysite/yandex/posfilter/leaf.cpp and
    //!       TURLBuffer::RemoveTrailingSlash() in ysite/yandex/common/prepattr.cpp
    inline bool HasAsterisk(const TUtf16String& s) {
        if (s.size() > 1 && s.back() == '*') {
            const wchar16 c = s[s.size() - 2];
            return (c == '/' || c == '\\' ? false : true);
        }
        return false;
    }

    inline void CreateUrlAttributeNode(TRequestNode& node, bool quoted, const TPosLen& pl, const TUtf16String& name, const TUtf16String& attrValue) {
        TUtf16String value(attrValue);
        if (quoted)
            value.erase(0, 1); // remove " (or any other char meaning ")
        Strip(value);
        const bool asterisk = HasAsterisk(value);
        if (asterisk)
            value.pop_back();
        value = PrepareURL(value);
        if (asterisk)
            value += '*';
        value.insert(size_t(0), 1, QUOTATION_CHAR); // insert " to the first position of the value
        SetAttributeNode(node, pl, name, value, TUtf16String());
    }

    inline bool CreateIntegerAttributeNode(TRequestNode& node, bool quoted, const TPosLen& pl, const TUtf16String& name, const TUtf16String& attrValue) {
        TUtf16String value(attrValue);
        // attribute must NEVER have double quote before text value
        if (quoted)
            value.erase(0, 1); // remove " (or any other char meaning ")

        TUtf16String loval;
        TUtf16String hival;
        if (!SplitInvervalValue(value, loval, hival)) {
            if (!CheckIntegerAttributeValue(value))
                return false;
            SetAttributeNode(node, pl, name, value, TUtf16String());
        } else {
            if (!CheckIntegerAttributeValue(loval) || !CheckIntegerAttributeValue(hival))
                return false;
            SetAttributeNode(node, pl, name, loval, hival);
        }
        return true;
    }

    inline void RestrictDateAttrByPos(TRequestNode::TFactory& nodeFactory, TRequestNode*& node, i32 loval, i32 hival) {
        Y_ASSERT(IsAttribute(*node));
        Y_ASSERT(node->GetText().size() == DATE_VALUE_LEN + 1); // "20090116 (with double-quote at the beginning)
        Y_ASSERT(loval < hival);

        TLangSuffix suffix = DefaultLangSuffix;
        suffix.OpInfo.Op = oRestrictByPos;
        suffix.OpInfo.Lo = loval;
        suffix.OpInfo.Hi = hival;
        node = nodeFactory.CreateBinaryOper(node, nullptr, DefaultLangPrefix, suffix, PHRASE_USEROP);
    }

    inline void SetDateAttrNode(TRequestNode* node, const TUtf16String& name, const TUtf16String& value, const TPosLen& pl) {
        Y_ASSERT(value.size() == DATE_VALUE_LEN); // 20090116
        //Y_ASSERT(value[0] != QUOTATION_CHAR);
        TUtf16String qvalue(value);
        qvalue.insert(size_t(0), 1, QUOTATION_CHAR);
        SetAttributeNode(*node, pl, name, qvalue, TUtf16String());
    }

    inline bool CreateDateAttributeNode(TRequestNode::TFactory& nodeFactory,
        TRequestNode*& node, bool quoted, const TPosLen& pl, const TUtf16String& name, const TUtf16String& attrValue)
    {
        TUtf16String value(attrValue);
        if (quoted)
            value.erase(0, 1); // remove " (or any other char meaning ")

        TUtf16String loval;
        TUtf16String hival;
        if (!SplitInvervalValue(value, loval, hival)) {
            switch (value.size())
            {
            case 4:     // date:2009
            case 5:     // date:2009*
                if (!CheckDateAttributeValue(value))
                    return false;
                loval.reserve(DATETIME_VALUE_LEN + 1);
                loval.append(QUOTATION_CHAR).append(value.c_str(), 4).append(JAN_01, Y_ARRAY_SIZE(JAN_01));
                loval.reserve(DATETIME_VALUE_LEN + 1);
                hival.append(QUOTATION_CHAR).append(value.c_str(), 4).append(DEC_31, Y_ARRAY_SIZE(DEC_31));
                SetAttributeNode(*node, pl, name, loval, hival);
                break;

            case 6:     // date:200904
            case 7:     // date:200904*
                if (!CheckDateAttributeValue(value))
                    return false;
                loval.reserve(DATETIME_VALUE_LEN + 1);
                loval.append(QUOTATION_CHAR).append(value.c_str(), 6).append(DAY_01, Y_ARRAY_SIZE(DAY_01));
                loval.reserve(DATETIME_VALUE_LEN + 1);
                hival.append(QUOTATION_CHAR).append(value.c_str(), 6).append(DAY_31, Y_ARRAY_SIZE(DAY_31));
                SetAttributeNode(*node, pl, name, loval, hival);
                break;

            case DATE_VALUE_LEN:    // date:20090408
                if (!CheckDateAttributeValue(value))
                    return false;
                SetDateAttrNode(node, name, value, pl);
                break;

            case DATETIME_VALUE_LEN:
                {
                    ui32 seconds = 0;
                    if (!GetDateTimeAttrValue(value, seconds))
                        return false;
                    SetDateAttrNode(node, name, value, pl);
                    RestrictDateAttrByPos(nodeFactory, node, seconds, seconds + 1);
                    break;
                }
            case DATETIME_NO_SECONDS_VALUE_LEN:
                {
                    ui32 seconds = 0;
                    if (!GetDateTimeNoSecondsAttrValue(value, seconds))
                        return false;
                    SetDateAttrNode(node, name, value, pl);
                    RestrictDateAttrByPos(nodeFactory, node, seconds, seconds + 60);
                    break;
                }
            case DATETIME_NO_MINUTES_VALUE_LEN:
                {
                    ui32 seconds = 0;
                    if (!GetDateTimeNoMinutesAttrValue(value, seconds))
                        return false;
                    SetDateAttrNode(node, name, value, pl);
                    RestrictDateAttrByPos(nodeFactory, node, seconds, seconds + 3600);
                    break;
                }
            default:
                return false;
            }
        } else {
            if (loval.size() != DATE_VALUE_LEN || hival.size() != DATE_VALUE_LEN)
                return false;
            if (!CheckDateAttributeValue(loval) || !CheckDateAttributeValue(hival))
                return false;
            loval.insert(size_t(0), 1, QUOTATION_CHAR);
            hival.insert(size_t(0), 1, QUOTATION_CHAR);
            SetAttributeNode(*node, pl, name, loval, hival);
        }

        return true;
    }

    inline bool CreateBooleanAttributeNode(TRequestNode& node, bool quoted, const TPosLen& pl, const TUtf16String& name, const TUtf16String& attrValue) {
        TUtf16String value(attrValue);
        if (quoted)
            value.erase(0, 1); // remove " (or any other char meaning ")

        if (!GetBooleanAttributeValue(value))
            return false;

        value.insert(size_t(0), 1, QUOTATION_CHAR); // insert " to the first position of the value
        SetAttributeNode(node, pl, name, value, TUtf16String());
        return true;
    }

    inline int CreateAttrNode(TReqTokenizer& tokenizer, const wchar16* tokend, void* lval, EReqAttrType type) {
        if (TReqAttrList::IsAttr(type)) {
            return CreateAttrNode(tokenizer, tokend, lval, false);
        } else {
            Y_ASSERT(type == RA_ATTR_SPECIAL);
            return CreateSpecialAttrNode(tokenizer, tokend, *(YYSTYPE*)lval);
        }
    }

    inline void GetParentsOfLeftmostAttr(TRequestNode* p, TRequestNode*& parent, TRequestNode*& grandparent) {
        while (p && p->Left) {
            if (IsAttribute(*p->Left)) {
                parent = (IsAndOp(*p) && p->OpInfo.Level == 2 ? p : nullptr); // only if it is the second space operator, see req_pars.y
                break;
            }
            grandparent = p;
            p = p->Left;
        }
    }
}

TRequestNode::TRequestNode(const TRequestNode& node, TFactory& factory)
    : TRequestNodeBase(node)
    , TIntrusiveListItem<TRequestNode>()
    , Left(node.Left ? factory.Clone(*node.Left) : nullptr)
    , Right(node.Right ? factory.Clone(*node.Right) : nullptr)
    , FormType(node.FormType)
    , Span(node.Span)
    , FullSpan(node.FullSpan)
{
    Softness = 0;
}

TRequestNode::~TRequestNode() {
    if (!Empty()) { // this is the root node
        Y_ASSERT(GC.Get() && GC->RefCount() == 1); // only root contains GC
        GC->Release(this);
    }
}

// leaf node (attribute or word) :
TRequestNode::TRequestNode(const TLangEntry& LE)
  : Softness(0)
  , Left(nullptr)
  , Right(nullptr)
{
    Y_ASSERT(LE.OpInfo.Lo == 0 && LE.OpInfo.Hi == 0 && LE.OpInfo.Op == oAnd && LE.OpInfo.Level == 0);
    OpInfo = ZeroDistanceOpInfo;
    FormType = LE.FormType;
    Necessity= LE.Necessity;
    ReverseFreq = LE.Idf;
    Span = TCharSpan(LE.EntryPos, LE.EntryLeng + LE.SuffixLeng);
    FullSpan = TCharSpan(LE.EntryPos, LE.EntryLeng + LE.SuffixLeng);
}

//! a sinlge multitoken contains words delimited with minus or apostrophe or a mark without suffixes and delimiters
static inline TPhraseType GetMultitokenPhraseType(const TWideToken& tok,
                                                  TTokenTypes tokenTypes)
{
    Y_ASSERT(!tok.SubTokens.empty());
    const size_t last = tok.SubTokens.size() - 1;
    if (last == 0 || IsClassifiedToken(tokenTypes)) {
        return PHRASE_NONE;
    }

    const TCharSpan& lastToken = tok.SubTokens[last];

    bool isMark = false;
    for (size_t j = 0; j < last; ++j) {
        if (tok.SubTokens[j].Type != lastToken.Type) {
            isMark = true;
            break;
        }
    }

    if (isMark) {
        for (size_t i = 0; i < last; ++i) {
            const TCharSpan& s = tok.SubTokens[i];
            if (s.SuffixLen != 0 || s.TokenDelim != TOKDELIM_NULL)
                return PHRASE_MULTISEQ;
            Y_ASSERT(s.EndPos() == tok.SubTokens[i + 1].Pos);
        }
        return PHRASE_MARKSEQ;
    } else if (lastToken.Type == TOKEN_WORD) {
        for (size_t i = 0; i < last; ++i) {
            const TCharSpan& s = tok.SubTokens[i];
            Y_ASSERT(s.Type == TOKEN_WORD);
            if (s.TokenDelim != TOKDELIM_APOSTROPHE && s.TokenDelim != TOKDELIM_MINUS)
                return PHRASE_MULTISEQ;
        }
        Y_ASSERT(tok.SubTokens[last].Type == TOKEN_WORD);
        return PHRASE_MULTIWORD;
    } else if (lastToken.Type == TOKEN_NUMBER) {
        for (size_t i = 0; i < last; ++i) {
            const TCharSpan& s = tok.SubTokens[i];
            Y_ASSERT(s.Type == TOKEN_NUMBER);
            if (s.TokenDelim != TOKDELIM_DOT)
                return PHRASE_MULTISEQ;
        }
        Y_ASSERT(tok.SubTokens[last].Type == TOKEN_NUMBER);
        return PHRASE_NUMBERSEQ;
    }
    Y_ASSERT(0);
    return PHRASE_MULTISEQ;
}

TRequestNode::TRequestNode(const TLangEntry& e,
                           const TWideToken& tok,
                           TTokenTypes tokenTypes)
  : Softness(0)
  , Left(nullptr)
  , Right(nullptr)
{
    OpInfo = ZeroDistanceOpInfo;

    if (!tok.SubTokens.size())
        ythrow yexception() << "multitoken has no subtokens";

    // corrects Multitoken (removes unary ops and replaces delimiters), assigns Name and TokSpan
    const TTokenStructure& subtokens = tok.SubTokens;
    Y_ASSERT(!subtokens.empty() && (subtokens[0].Pos - subtokens[0].PrefixLen) == 0 && tok.Token >= e.Text);
    Span = TCharSpan(tok.Token - e.Text, tok.Leng); // prefix/suffix included, no prefix ops
    Prefix.assign(tok.Token, subtokens[0].PrefixLen);

    // replace delimiters with ascii characters
    TUtf16String text(tok.Token, tok.Leng);
    for (size_t i = 0; i < subtokens.size() - 1; ++i) {
        const TCharSpan& s = subtokens[i];
        const TCharSpan& next = subtokens[i + 1];
        size_t seplen = next.Pos - next.PrefixLen - (s.EndPos() + s.SuffixLen);
        if (seplen && s.TokenDelim != TOKDELIM_UNKNOWN) { // TOKDELIM_UNKNOWN used for concatenated multitokens
            Y_ASSERT(s.EndPos() + s.SuffixLen < text.size() && seplen == 1);
            text.replace(s.EndPos() + s.SuffixLen, 1, 1, GetTokenDelimChar(s.TokenDelim));
        }
    }

    TPhraseType pt = GetMultitokenPhraseType(tok, tokenTypes);
    SetLeafWord(text, TCharSpan(0, subtokens.back().EndPos()), pt);
    SubTokens = subtokens;

    FormType = e.FormType;
    Necessity= e.Necessity;
    ReverseFreq = e.Idf;

    if (IsClassifiedToken(tokenTypes)) {
        TokenTypes = tokenTypes;
    }
}

// binary oper:
TRequestNode::TRequestNode(TRequestNode* left, TRequestNode* right,
                           const TLangPrefix& lp, const TLangSuffix &ls, TPhraseType ft)
    : Softness(0)
    , Left(left)
    , Right(right)
{
    OpInfo = ls.OpInfo;
    Y_ASSERT(ft == PHRASE_PHRASE || ft == PHRASE_USEROP);
    SetPhraseType(ft);
    Necessity= lp.Necessity;
    FormType = lp.FormType;
    ReverseFreq = ls.Idf;

    if (ft == PHRASE_USEROP && OpInfo.Op != oRefine && OpInfo.Op != oOr && OpInfo.Op != oWeakOr && OpInfo.Op != oRestrDoc) {
        if (left && left->OpInfo.Op == oRestrDoc)
            left->Parens = true;
        if (right && right->OpInfo.Op == oRestrDoc)
            right->Parens = true;
    }

    if (left && right) {
        Span.Pos = Min(left->Span.Pos, right->Span.Pos);
        Span.Len = Max(right->Span.EndPos(), left->Span.EndPos()) - Span.Pos;
        FullSpan.Pos = left->FullSpan.Pos;
        FullSpan.Len = right->FullSpan.EndPos() - left->FullSpan.Pos;
    } else if (left) {
        Span = left->Span;
        FullSpan = left->FullSpan;
    } else if (right) {
        Span = right->Span;
        FullSpan = right->FullSpan;
    }
}

TRequestNode* ConvertAZNode(TRequestNode* pNode, TRequestNode* right)
{
    Y_ASSERT(pNode);
    Y_ASSERT(pNode->Right == nullptr);
    if (right == nullptr) {
        pNode->ConvertNameToText();
        return pNode;
    }

    if (right->OpInfo.Op == oRestrDoc) {
        right->Left = ConvertAZNode(pNode, right->Left);
        return right;
    }

    size_t fullSpanEnd = Max(pNode->FullSpan.EndPos(), right->FullSpan.EndPos());
    pNode->FullSpan.Len = fullSpanEnd - pNode->FullSpan.Pos;

    TRequestNode* parent = nullptr;
    TRequestNode* grandparent = nullptr;
    GetParentsOfLeftmostAttr(right, parent, grandparent);

    pNode->OpInfo.Op = oZone;

    // Adjust suffix and prefix

    // @todo do not apply necessity and form type of the left node to the right
    //      ex. -!!title[text], text will not have -!!

    // @todo reset necessity or form type of the left node if the right node has it
    //      ex. -!!title[+!text], necessity and form type of title will be reset: title[+!text]
    //      there is a problem: title#attr=+!value [text] -> title#+!attr=value [text] but this request is invalid

    if (right->FormType == fGeneral)
        right->FormType = pNode->FormType; // @todo remove the assignment
    else
        pNode->FormType = right->FormType; // @todo reset to fGeneral

    if (right->Necessity == nDEFAULT)
        right->Necessity = pNode->Necessity; // @todo remove the assignment
    else
        pNode->Necessity = right->Necessity; // @todo reset to nDEFAULT

    if (!pNode->OpInfo.Arrange)
        pNode->OpInfo.Arrange = right->OpInfo.Arrange;
    if (!IsZone(*right))
        right->OpInfo.Arrange = false;

    if (parent) {
        if (parent == right) {
            // z:(a:"val" b) -> z#a="val"[b]
            pNode->Left = right->Left;
            right->Left = nullptr;
            pNode->Right = right->Right;
            right->Right = nullptr;
        } else {
            Y_ASSERT(grandparent);
            // z:(a:"val" b c d e) -> z#a="val"[b c]
            pNode->Left = parent->Left;
            parent->Left = nullptr;
            pNode->Right = right;
            pNode->Right->Parens = false; // since in the previous case Right is removed - Parens is reset here
            grandparent->Left = parent->Right;
            parent->Right = nullptr;
        }
    } else {
        pNode->Right = right;
    }
    return pNode;
}

TUtf16String GetNameByEntry(const TLangEntry& entry) {
    return TUtf16String(entry.Text + entry.EntryPos, entry.EntryLeng + entry.SuffixLeng);
}

int CreateZoneNode(const TReqTokenizer& tokenizer, const wchar16* tokend, void* lval) {
    const TLangEntry& entry = tokenizer.GetEntry();
    const TPosLen pl = GetAttrValuePosLen(entry, tokend);

    TRequestNode* zone(CreateZoneNode(tokenizer, entry));
    Y_ASSERT(zone);
    zone->FullSpan = TCharSpan(entry.EntryPos, tokend - entry.Entry());

    // entry.Arrange is ignored in CreateWordNode
    TRequestNode* word(ParseText(tokenizer, entry.Text, pl.first, pl.second));
    if (!word) {
        ((YYSTYPE*)lval)->pnode = nullptr;
        return ATTR_VALUE;
    }
    word->FullSpan = TCharSpan(entry.EntryPos, tokend - entry.Entry());

    ((YYSTYPE*)lval)->pnode = ConvertAZNode(zone, word);
    return ATTR_VALUE;
}

TRequestNode* CreateZoneNode(const TReqTokenizer& tokenizer, const TLangEntry& e) {
    TRequestNode* node(tokenizer.GetNodeFactory().CreateAttrTypeNode(e));
    Y_ASSERT(node);
    Y_ASSERT(e.OpInfo.CmpOper == cmpEQ); // ':' allowed only for zones, no any other operators (>, >=, <, <=)
    TUtf16String name = GetNameByEntry(e);
    Y_ASSERT(name.size() == e.EntryLeng);
    node->SetZoneName(name, e.OpInfo.Arrange);
    //Y_ASSERT(IsZone(*node)); it is not a zone node yet... the call to CreateAZNode should follow next
    return node;
}

int CreateAttrNode(const TReqTokenizer& tokenizer, const wchar16* tokend, void* lval, bool quoted) {
    const TLangEntry& entry = tokenizer.GetEntry();

    // only the most top node is created on TLangEntry
    TRequestNode* node(tokenizer.GetNodeFactory().CreateAttrTypeNode(entry));
    Y_ASSERT(node);
    Y_ASSERT(entry.OpInfo.Arrange == false);
    node->OpInfo.CmpOper = entry.OpInfo.CmpOper;
    TUtf16String name = GetNameByEntry(entry);
    Y_ASSERT(name.size() == entry.EntryLeng);

    // Adjust the span of the topmost node
    Y_ASSERT(tokend >= entry.Entry());
    node->Span.Len = tokend - entry.Entry();

    const EReqAttrType type = tokenizer.request.GetAttrType(name);
    Y_ASSERT(TReqAttrList::IsAttr(type));

    const TPosLen pl = GetAttrValuePosLen(entry, tokend);
    TUtf16String attrValue(entry.Text + pl.first, pl.second - (quoted ? 1 : 0)); // if quoted is true - remove the right quote or parenthesis

    switch (type) {
    case RA_ATTR_LITERAL:
        CreateLiteralAttributeNode(*node, quoted, pl, name, attrValue);
        break;
    case RA_ATTR_URL:
        CreateUrlAttributeNode(*node, quoted, pl, name, attrValue);
        break;
    case RA_ATTR_LITERAL_URL:
        CreateLiteralUrlAttributeNode(*node, quoted, pl, name, attrValue);
        break;
    case RA_ATTR_INTEGER:
        if (!CreateIntegerAttributeNode(*node, quoted, pl, name, attrValue))
            node = ConvertAttrNodeToWordNodes(tokenizer, entry, pl);
        break;
    case RA_ATTR_DATE:
        if (!CreateDateAttributeNode(tokenizer.GetNodeFactory(), node, quoted, pl, name, attrValue))
            node = ConvertAttrNodeToWordNodes(tokenizer, entry, pl);
        break;
    case RA_ATTR_BOOLEAN:
        if (!CreateBooleanAttributeNode(*node, quoted, pl, name, attrValue))
            node = ConvertAttrNodeToWordNodes(tokenizer, entry, pl);
        break;
    default:
        Y_ASSERT(!"Unexpected node type.");
        return 0;
    }

    node->FullSpan = TCharSpan(entry.EntryPos, tokend - entry.Entry());

    ((YYSTYPE*)lval)->pnode = node;
    return ATTR_VALUE;
}

TRequestNode* ParseQuotedString(TReqTokenizer& reqTokenizer, const TLangEntry& extLE)
{
    THolder<TQuotedStringTokenizer> quotedStringTokenizer(reqTokenizer.GetQuotedStringTokenizer(reqTokenizer.request,
                                                 reqTokenizer.GetNodeFactory(),
                                                 extLE,
                                                 reqTokenizer.GetProccessedTokensLengthInfo()));
    TRequestNode *phrase = quotedStringTokenizer->Parse();

    if (phrase) {
        phrase->Necessity = extLE.Necessity;
        SetSuffix(*phrase, extLE.Idf);
        if (phrase->Necessity == nDEFAULT && phrase->FormType == fGeneral)
            phrase->Parens = false;
        if (extLE.OpInfo.Op == oAnd) {
            phrase->FormType = extLE.FormType;
            phrase->SetQuoted();
        }

        phrase->FullSpan = TCharSpan(extLE.EntryPos, extLE.EntryLeng);
    } else {
        // 2DO: error handling !
    }

    return phrase;
}

IOutputStream& operator<<(IOutputStream& out, const TOperType& operType) {
   return out << ToString(operType);
}

IOutputStream &operator <<(IOutputStream &out, const TFormType& ft) {
    return out << ToString(ft);
};

IOutputStream &operator <<(IOutputStream &out, const TCompareOper& compareOper) {
    return out << ToString(compareOper);
};

IOutputStream &operator <<(IOutputStream &out, const TNodeNecessity& nn) {
    return out << (int)nn;
};

IOutputStream &operator <<(IOutputStream &out, const TPhraseType& phraseType) {
   return out << ToString(phraseType);
}

IOutputStream& printNode(IOutputStream &os, const TRequestNode *n, int depth) {
   if (!n) {
      os << "#\n";
      return os;
   }
   os << "[" << n->Span.Pos << ":" << n->Span.EndPos() << "] ";
   if (IsWordOrMultitoken(*n)) {
       os << "Op:oWord";
   } else if (IsAttribute(*n)) {
       os << "Op:oAttr";
   } else
       os << "Op:" << n->Op();
   if (n->FormType != fGeneral)
      os << ", " << n->FormType;
   os << " Level:" <<  n->OpInfo.Level;
   os << " Lo:" << n->OpInfo.Lo;
   os << " Hi:" << n->OpInfo.Hi;
   if (IsAttribute(*n))
      os << " CmpOper:" << n->OpInfo.CmpOper;
   if (n->OpInfo.Arrange) {
      os << " Arrange:" << n->OpInfo.Arrange;
   }

   if (!!n->GetTextName())
      os << " Name:" << n->GetTextName();
   else if (!!n->GetText()) {
      os << " Name:";
      const TWideToken tok = n->GetMultitoken();
      for (size_t i = 0; i < tok.SubTokens.size(); ++i) {
         const TCharSpan& subtok = tok.SubTokens[i];
         if (subtok.PrefixLen) {
            const size_t start = subtok.Pos - subtok.PrefixLen;
            os << '(' << TWtringBuf(tok.Token + start, subtok.PrefixLen) << ')';
         }
         os << "[" << TWtringBuf(tok.Token + subtok.Pos, subtok.Len) << ']';
         if (subtok.SuffixLen) {
            const size_t start = subtok.Pos + subtok.Len;
            os << '(' << TWtringBuf(tok.Token + start, subtok.SuffixLen) << ')';
         }
         if (subtok.TokenDelim != TOKDELIM_NULL && (i + 1 < tok.SubTokens.size())) {
            const size_t start = subtok.Pos + subtok.Len + subtok.SuffixLen;
            const TCharSpan& next = tok.SubTokens[i + 1];
            os << TWtringBuf(tok.Token + start, next.Pos - next.PrefixLen - start);
         }
      }
   }
   if (!!n->GetFactorName()) {
       os << " Factor:" << n->GetFactorName() << " FactorValue:" << n->GetFactorValue();
   }

   if (n->GetPhraseType() != PHRASE_NONE)
      os << " PhraseType:" << n->GetPhraseType();
   if (n->IsQuoted())
      os << " Quoted";

   if (IsAttribute(*n)) {
      os << " AttrV:"  << n->GetText();
      if (!!n->GetAttrValueHi())
         os << " AttrVHi:" << n->GetAttrValueHi();
   }
   if (n->ReverseFreq >=0)
      os << " ReverseFreq:" << n->ReverseFreq;
   if (n->Parens)
      os << " SubExpr:sePARENS";
   if (n->GetSoftness())
      os << " Softness:" << n->GetSoftness();
   if (n->Necessity != nDEFAULT)
      os << " Necessity:" << n->Necessity;
   os << " Span: [" << n->Span.Pos << ", " << n->Span.EndPos() << ") ";
   os << " FullSpan: [" << n->FullSpan.Pos << ", " << n->FullSpan.EndPos() << ")";

   os << "#\n";
   int i;
   for (i = 0; i<=depth; ++i)
      os << "   ";
   printNode(os,n->Left,depth+1);
   for (i = 0; i<=depth; ++i)
      os << "   ";
   printNode(os,n->Right,depth+1);
   return os;
}

IOutputStream &operator << (IOutputStream &os,const TRequestNode *n) {
   return printNode(os, n, 0);
}

void TRequestNode::TreatPhrase(const TOpInfo& phraseOperInfo, TPhraseType defaultType) {
    Y_ASSERT(phraseOperInfo.Op == oAnd);
    Y_ASSERT(defaultType == PHRASE_PHRASE || defaultType == PHRASE_USEROP);
    if (PHRASE_PHRASE == GetPhraseType()) {
        Y_ASSERT(OpInfo.Op == oAnd);
        if (!(OpInfo.Lo == 1 && OpInfo.Hi == 1 && OpInfo.Level == 0))
            SetOpInfo(phraseOperInfo);
        SetPhraseType(defaultType);
    }
    if (Left)
        Left->TreatPhrase(phraseOperInfo, defaultType);
    if (Right)
        Right->TreatPhrase(phraseOperInfo, defaultType);
}

int CreateZoneOrAttr(TReqTokenizer& tokenizer, const wchar16* tokend, void* lval) {
    Y_ASSERT(lval);
    const EReqAttrType type = tokenizer.request.GetAttrType(GetNameByEntry(tokenizer.GetEntry()));
    return (type == RA_ZONE ? CreateZoneNode(tokenizer, tokend, lval) : CreateAttrNode(tokenizer, tokend, lval, type));
}

TRequestNode* ConvertZoneExprToPhrase(TRequestNode* badZone, TRequestNode* expr, const TReqTokenizer& tokenizer)
{
    Y_ASSERT(badZone);
    if (!expr)
        return nullptr;
    badZone->ConvertNameToText();
    TLangSuffix ls = DefaultLangSuffix;
    ls.OpInfo.Op = oAnd;
    return tokenizer.GetNodeFactory().CreateBinaryOper(badZone, expr, DefaultLangPrefix, ls, PHRASE_PHRASE);
}
