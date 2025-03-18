#include "hx_sanitize.h"

#include <fstream>
#include <iostream>

#include <util/system/compat.h>
#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <util/string/strip.h>

#include <library/cpp/html/entity/htmlentity.h>
#include <library/cpp/html/sanitize/css/class_sanitize.h>
#include <library/cpp/html/sanitize/css/css_sanitize.h>

TString HtEntDecodeToUtf(ECharset fromCharset, const char* str, size_t strLen) {
    TCharTemp wide(strLen * 2); // if we have 1-byte charset that's all mapped to pairs
    size_t outLen = HtEntDecodeToChar(fromCharset, str, strLen, wide.Data());
    return WideToUTF8(wide.Data(), outLen);
}

/********************************************************/
HXAttributeChecker::HXAttributeChecker(const char* baseURL)
    : resolveRelativeUrls(true)
{
    setBaseUrl(baseURL);
}

/********************************************************/
bool HXAttributeChecker::setBaseUrl(const char* baseURL) {
    if (resolveRelativeUrls && baseURL) {
        resolveRelativeUrls = strcmp(baseURL, "no_resolve");
        mBaseURL.Parse(baseURL,
                       THttpURL::FeatureSchemeFlexible |
                           THttpURL::FeaturePathOperation |
                           THttpURL::FeatureNormalPath |
                           THttpURL::FeatureTryToFix);
    } else {
        mBaseURL.Clear();
    }
    return mBaseURL.IsValidGlobal();
}

/********************************************************/
bool HXAttributeChecker::checkURL(const char*& valStart,
                                  int& valLength,
                                  TString& urlBufValue) {
    TString v(valStart, 0, valLength);

    THttpURL the_url;

    int parse_err = the_url.ParseUri(v,
                                     THttpURL::FeatureSchemeFlexible |
                                         THttpURL::FeaturePathOperation |
                                         THttpURL::FeatureNormalPath |
                                         THttpURL::FeatureTryToFix);

    switch (parse_err) {
        case THttpURL::ParsedEmpty:
            the_url.Clear();
            break;
        case THttpURL::ParsedOpaque:
            return true;

        case THttpURL::ParsedOK:
            break;

        default:
            return false;
    }

    if (the_url.GetScheme() == THttpURL::SchemeUnknown || the_url.IsValidGlobal())
        return true;
    if (v.StartsWith("//")) {
        the_url.SetScheme(THttpURL::SchemeHTTPS);
    } else if (!mBaseURL.IsValidGlobal() && resolveRelativeUrls) {
        TString urlv = the_url.PrintS();
        const TString tmpScheme = "http://";
        if (!urlv.empty() && !urlv.Contains("://")) {
            urlBufValue = tmpScheme + urlv;
            valStart = urlBufValue.c_str();
            valLength = (int)urlBufValue.length();
            return true;
        }
        return false;
    }

    if (resolveRelativeUrls) {
        the_url.Merge(mBaseURL);
        if (!the_url.IsValidGlobal()) {
            return false;
        }
    }
    urlBufValue = the_url.PrintS();
    valStart = urlBufValue.c_str();
    valLength = (int)urlBufValue.length();
    return true;
}

/********************************************************/
/********************************************************/
hxWriter::hxWriter(ECharset cp, int lineLimit)
    : mCodePageIn(cp)
    , mLineLimit(lineLimit)
    , mNobrPrevSpace(false)
    , mBuf(nullptr)
    , mBufEnd(nullptr)
    , mBufLast(nullptr)
    , mBufCur(nullptr)
    , mBufPoint(nullptr)
    , mCurLineLength(0)
    , mPointLineLength(0)
    , mPointSpaceMode(HX_SPACE_IGNORE)
{
    mBuf = new unsigned char[4096];
    mBufCur = mBuf;
    mBufEnd = mBuf + 4000;
    mBufLast = mBuf + 4095;
}

/********************************************************/
hxWriter::~hxWriter() {
    assert(mBufCur == mBuf);
    delete[] mBuf;
}

/********************************************************/
void hxWriter::dropLine() {
    mCurLineLength = mPointLineLength = 0;
    mBufPoint = nullptr;
    mPointSpaceMode = HX_SPACE_IGNORE;
}

/********************************************************/
void hxWriter::writeBreak(hxSpaceMode mode) {
    if (mode == HX_SPACE_LITERAL) {
        doWrite("\n", 1);
    } else {
        assert(mode == HX_SPACE_PRE ||
               mode == HX_SPACE_NOBR);
        static const char br[] = "<br sanitizer=\"yes\"/>";
        doWrite(br, sizeof(br) - 1);
    }
}

/********************************************************/
bool hxWriter::writeText(const char* buf,
                         size_t size,
                         hxSpaceMode spaceMode) {
    const unsigned char* s = (const unsigned char*)(buf);
    const unsigned char* end = s + size;
    bool ret = true;

    while (s < end) {
        if (mBufCur >= mBufEnd)
            flush(false);

        wchar32 w;

        // Determine symbol
        if (spaceMode == HX_SPACE_ATTR)
            w = HtEntOldPureDecodeStep(mCodePageIn, s, end - s, nullptr);
        else
            w = HtEntOldDecodeStep(mCodePageIn, s, end - s, nullptr);

        // Bad XML symbols
        // http://www.w3.org/TR/2008/REC-xml-20081126/#charsets
        if (((w < 0x20) && !((w == 0x9) || (w == 0xA) || (w == 0xD))) ||
            ((w > 0xD7FF) && (w < 0xE000)) ||
            ((w > 0xFFFD) && (w < 0x10000)) ||
            (w > 0x10FFFF))
            continue;

        // Try break line
        if (spaceMode < HX_SPACE_IGNORE && mLineLimit > 10 && mCurLineLength >= mLineLimit) {
            if (spaceMode == HX_SPACE_NORMAL) {
                static const char wbr[] = "<wbr sanitizer=\"yes\"/>";
                writeSymbols(wbr, sizeof(wbr) - 1);
                dropLine();
            } else {
                if (!IsSpace(w)) {
                    if (mBufPoint) {
                        if (mBufPoint > mBuf) {
                            int len = int(mBufPoint - mBuf);
                            doWrite((const char*)(mBuf), len);
                            memmove(mBuf, mBufPoint,
                                    int(mBufCur - mBufPoint));
                            mBufPoint -= len;
                            mBufCur -= len;
                        }
                        writeBreak(mPointSpaceMode);
                        mBufPoint = nullptr;
                        mCurLineLength = mPointLineLength;
                        mPointLineLength = 0;
                    } else {
                        flush(true);
                        writeBreak(spaceMode);
                        dropLine();
                    }
                }
            }
        }

        // Write symbol
        switch (w) {
            case '<':
                writeSymbols("&lt;", 4);
                break;
            case '>':
                writeSymbols("&gt;", 4);
                break;
            case '&':
                writeSymbols("&amp;", 5);
                break;
            case '"':
                if (spaceMode == HX_SPACE_ATTR) {
                    writeSymbols("&quot;", 6);
                    break;
                }
                [[fallthrough]];
            default: {
                if (w == BROKEN_RUNE)
                    break;

                size_t l0;
                if (SafeWriteUTF8Char(w, l0, mBufCur, mBufLast) == RECODE_OK) {
                    mBufCur += l0;
                } else {
                    ret = false;
                }
            }
        }

        // Work with white spaces
        if (IsSpace(w)) {
            if (spaceMode != HX_SPACE_NOBR || !mNobrPrevSpace) {
                mCurLineLength++;
                mPointLineLength++;
            }
            mNobrPrevSpace = (spaceMode == HX_SPACE_NOBR);
            if (spaceMode == HX_SPACE_NORMAL) {
                if (w != 0xA0)
                    dropLine();
            } else if (spaceMode >= HX_SPACE_PRE && spaceMode <= HX_SPACE_LITERAL) {
                if (w == '\n') {
                    dropLine();
                } else {
                    mBufPoint = mBufCur;
                    mPointLineLength = 0;
                    mPointSpaceMode = spaceMode;
                }
            }
        } else {
            mCurLineLength++;
            mPointLineLength++;
            mNobrPrevSpace = false;
        }
    }

    return ret;
}

/********************************************************/
void hxWriter::writeSymbols(const char* buf, size_t buflen) {
    if (mBufCur + buflen >= mBufEnd)
        flush(false);

    if (mBufCur + buflen >= mBufEnd) {
        flush(true);
        doWrite(buf, (int)buflen);
    } else {
        memcpy(mBufCur, buf, buflen);
        mBufCur += buflen;
    }
}

/********************************************************/
void hxWriter::flush(bool complete) {
    if (mBufCur == mBuf)
        return;

    if (mBufPoint && (complete || mBufPoint < mBuf + 2000))
        mBufPoint = nullptr;

    if (!mBufPoint) {
        doWrite((const char*)(mBuf), int(mBufCur - mBuf));
        mBufCur = mBuf;
    } else {
        int len = int(mBufPoint - mBuf);
        doWrite((const char*)(mBuf), len);
        mBufCur -= len;
        if (mBufCur > mBuf)
            memmove(mBuf, mBufPoint, int(mBufCur - mBuf));
        mBufPoint = mBuf;
    }
}

/********************************************************/
/********************************************************/
hxWriterForStroka::hxWriterForStroka(ECharset cp, TString& str)
    : hxWriter(cp, 0)
    , mStroka(str)
{
}

/********************************************************/
hxWriterForStroka::~hxWriterForStroka() {
    flush();
}

/********************************************************/
void hxWriterForStroka::doWrite(const char* buf, int buflen) {
    mStroka.append(buf, 0, buflen, buflen);
}

size_t hxWriterForStroka::getOffset() {
    return mBufCur - mBuf;
}

/********************************************************/
/********************************************************/
hxWriterForSanitizer::hxWriterForSanitizer(ECharset cp,
                                           HXSanitizer& sanitizer,
                                           int line_limit)
    : hxWriter(cp, line_limit)
    , mSanitizer(sanitizer)
{
}

/********************************************************/
hxWriterForSanitizer::~hxWriterForSanitizer() {
    flush();
}

/********************************************************/
void hxWriterForSanitizer::doWrite(const char* buf, int buflen) {
    mSanitizer.writeOutput(buf, buflen);
}

/********************************************************/
/********************************************************/
const NHtml::TTag* HXSanitizer::sTagA = &NHtml::FindTag("A", 1);
const NHtml::TTag* HXSanitizer::sTagFIELDSET = &NHtml::FindTag("FIELDSET", 8);
const NHtml::TTag* HXSanitizer::sTagTBODY = &NHtml::FindTag("TBODY", 5);
const NHtml::TTag* HXSanitizer::sTagTD = &NHtml::FindTag("TD", 2);
const NHtml::TTag* HXSanitizer::sTagTR = &NHtml::FindTag("TR", 2);
const NHtml::TTag* HXSanitizer::sTagLI = &NHtml::FindTag("LI", 2);
const NHtml::TTag* HXSanitizer::sTagUL = &NHtml::FindTag("UL", 2);

/********************************************************/
HXSanitizer::HXSanitizer(NHtmlLexer::TObsoleteLexer& lexer,
                         HXAttributeChecker* checker,
                         ECharset cpIn,
                         int lineLimit,
                         Yandex::NCssSanitize::TClassSanitizer* classSanitizer,
                         Yandex::NCssSanitize::TCssSanitizer* cssSanitizer,
                         Yandex::NCssSanitize::TFormSanitizer* formSanitizer,
                         TString css_config_file,
                         int depth)
    : mLexer(lexer)
    , mChecker((checker) ? checker : hxMakeAttributeChecker())
    , mWriter(cpIn, *this, lineLimit)
    , mTags()
    , mCodePageIn(cpIn)
    , mIMStack(nullptr)
    , mBodyAttrs(nullptr)
    , mOutBodyTag(nullptr)
    , mOutTitle(nullptr)
    , mLastEmptyOutput(false)
    , mFailed(false)
    , mAttrSet()
    , mClassSanitizer(classSanitizer)
    , mCssSanitizer(cssSanitizer)
    , mFormSanitizer(formSanitizer)
    , mcss_config_file(css_config_file)
    , mNestingDepth(depth)
    , mCurDepth(0)
    , mIgnoreTo(nullptr)
    , mIgnoreDepth(0)
    , mLength(0)
{
    UrlProcessor = nullptr;
    FilterEntities = nullptr;
    MetaContainer = nullptr;

    if (classSanitizer != nullptr) {
        mClassSanitizer = classSanitizer;
        ownClassSanitizer = false;
    } else {
        mClassSanitizer = new Yandex::NCssSanitize::TClassSanitizer;
        mClassSanitizer->OpenConfigString("default pass  class deny ( /^.+-c-.+/ )");
        ownClassSanitizer = true;
    }

    if (cssSanitizer != nullptr) {
        mCssSanitizer = cssSanitizer;
        ownCssSanitizer = false;
    } else if (!!mcss_config_file) {
        mCssSanitizer = new Yandex::NCssSanitize::TCssSanitizer;
        mCssSanitizer->OpenConfigString(mcss_config_file);
        ownCssSanitizer = true;
    } else
        ownCssSanitizer = false;

    if (mChecker)
        mChecker->mCodePage = mCodePageIn;
}

/********************************************************/
HXSanitizer::~HXSanitizer() {
    mWriter.flush();
    freeIM();

    if (mBodyAttrs)
        delete mBodyAttrs;

    if (ownClassSanitizer)
        delete mClassSanitizer;

    if (ownCssSanitizer)
        delete mCssSanitizer;

    delete mChecker;
}

/********************************************************/
bool HXSanitizer::myAssert(bool condition) {
    assert(condition);
    if (!condition) {
        mFailed = true;
        freeIM();
    }
    return condition;
}

/********************************************************/

const NHtml::TTag* HXSanitizer::getCurrentTag(bool isOpenTag) {
    const char* curTag = mLexer.getCurTag();
    const unsigned int len = mLexer.getCurTagLength();
    if (FilterEntities) {
        const TString tagName = FilterEntities->ReplaceTag(curTag, len);
        return identifyTag(tagName.data(), tagName.size(), isOpenTag);
    }
    return identifyTag(curTag, len, isOpenTag);
}

/********************************************************/
bool HXSanitizer::doIt(bool irregularMarkup,
                       TString* outBodyTag,
                       TString* outTitle) {
    freeIM();
    if (irregularMarkup)
        startIM();

    mOutBodyTag = outBodyTag;
    mOutTitle = outTitle;
    if (mOutBodyTag) {
        *mOutBodyTag = "<body";
        if (mBodyAttrs)
            delete mBodyAttrs;
        mBodyAttrs = new hxAttrSet();
    }

    if (mOutTitle) {
        *mOutTitle = "";
    }

    HTLEX_TYPE eT;

    while (true) {
        eT = mLexer.Next();
        switch (eT) {
            case HTLEX_EOF:
                break;

            case HTLEX_START_TAG: {
                const NHtml::TTag* tag = getCurrentTag(true);
                if (tag) {
                    if (!mIgnoreTo) {
                        unsigned processFlags = 0;
                        openTag(tag, processFlags);
                        if (tag->is(HT_empty) == false)
                            break;
                        if (mIgnoreTo == nullptr)
                            break;
                        if (strcmp(mIgnoreTo, tag->name) != 0)
                            break;
                        mIgnoreDepth = 0;
                        mIgnoreTo = nullptr;
                        closeTag(tag);
                    } else if (strcmp(mIgnoreTo, tag->name) == 0)
                        ++mIgnoreDepth;
                } else {
                    TString text(mLexer.GetCurText(), mLexer.GetCurLength());
                    if (FilterEntities && FilterEntities->IsAcceptedAngBrContent(text.data()))
                        doText();
                }
            } break;

            case HTLEX_END_TAG: {
                const NHtml::TTag* tag = getCurrentTag(false);
                if (tag) {
                    if (mIgnoreTo) {
                        if (strcmp(mIgnoreTo, tag->name) == 0) {
                            if (--mIgnoreDepth == 0) {
                                mIgnoreTo = nullptr;
                                closeTag(tag);
                            }
                        }
                    } else
                        closeTag(tag);
                }
            } break;

            case HTLEX_TEXT: {
                TString text(mLexer.GetCurText(), mLexer.GetCurLength());
                if (!mIgnoreTo && (!FilterEntities || !FilterEntities->IsBadMarkup(text.data())))
                    doText();
            } break;

            default:
                break;
        }

        if (eT == HTLEX_EOF)
            break;
    }

    bool needsNL = false;

    while (!mTags.empty() && (mLexer.good() || mLexer.eof())) {
        closeOneTag();
        needsNL = true;
    }

    if (needsNL && !mLastEmptyOutput)
        mWriter.writeSymbols("\n", 1);
    mWriter.flush();

    if (mOutBodyTag) {
        for (unsigned i = 0; i < mBodyAttrs->size(); i++) {
            (*mOutBodyTag) += (*mBodyAttrs)[i].mPrint;
        }

        *mOutBodyTag += ">";
    }

    return mLexer.good() || mLexer.eof();
}

/********************************************************/
void HXSanitizer::formDocument(std::istream& inProcessedDoc,
                               TString& inBodyTag,
                               TString& inTitle,
                               std::ostream& outDoc) {
    outDoc << "<html>\n <head>\n  <title>" << inTitle.c_str()
           << "</title>\n "
           << "<meta http-equiv=\"Content-Type\""
           << " content=\"text/html; charset=utf-8\">\n"
           << "</head>\n"
           << inBodyTag.c_str() << "\n";

    do {
        char buf[10024];
        inProcessedDoc.read(buf, 1023);
        if (inProcessedDoc.gcount() > 0)
            outDoc.write(buf, inProcessedDoc.gcount());
    } while (!inProcessedDoc.eof() && !inProcessedDoc.bad());

    outDoc << "\n</body>\n</html>"
           << "\n";
}

/********************************************************/
bool HXSanitizer::openTag(const NHtml::TTag* tag, unsigned processFlags) {
    hxParseState state(tag, 0);

    if (mFormSanitizer && tag->is(HT_FORM))
        for (int i = 0; i < mLexer.getCurNAtrs(); i++) {
            int nameLen, valLen;
            const char* name;
            const char* val;
            if (mLexer.getCurAttrName(i, name, nameLen)) {
                TString nm(name, nameLen);
                nm.to_lower();
                if (nm == "action")
                    if (mLexer.getCurAttrValue(i, val, valLen))
                        if (!mFormSanitizer->Pass(TString(val, valLen)))
                            return false;
            }
        }

    if (!doOpenTag(state, processFlags))
        return false;

    if (mWriter.activeLineCut() && (tag->is(HT_TD) || tag->is(HT_TD))) {
        // Check for <td nowrap>
        for (int i = 0; i < mLexer.getCurNAtrs(); i++) {
            int l;
            const char* v;
            if (mLexer.getCurAttrName(i, v, l) &&
                l == 6 &&
                !strnicmp("nowrap", v, 6)) {
                state.mSpaceMode = HX_SPACE_PRE;
                break;
            }
        }
    }

    if (state.is(hxIgnoreTag) && !noSanitize()) {
        if (tag->is(HT_BASE) && mChecker) {
            for (int i = 0; i < mLexer.getCurNAtrs(); i++) {
                int l;
                const char* v;
                if (mLexer.getCurAttrName(i, v, l) &&
                    l == 4 &&
                    !strnicmp("href", v, 4)) {
                    if (mLexer.getCurAttrValue(i, v, l)) {
                        TString vv(v, 0, l);
                        mChecker->setBaseUrl(vv.c_str());
                    }
                }
            }
        }
        return false;
    }

    if (state.is(hxIgnoreWithChild) && !noSanitize()) {
        if (!mIgnoreTo) {
            mIgnoreTo = tag->name;
            mIgnoreDepth = 1;
        }
        return false;
    }

    if (!(processFlags & hxProcessFlagNeedsClose))
        mTags.push_back(state);

    if (state.is(hxIgnoreContent) && !noSanitize()) {
        return false;
    }

    if (tag->is(HT_irreg) && inIM()) {
        if (myAssert(!(processFlags & (hxProcessFlagAuto | hxProcessFlagNeedsClose)))) {
            hxIMTag t(tag);
            {
                hxWriterForStroka wr(mCodePageIn, t.mPrint);
                writeTagWithAttrs(wr, tag->lowerName, state, true, false);
            }
            addToIM(t);
        }
    } else {
        //time to make output
        onWrite(HTLEX_START_TAG, processFlags);
        writeTagWithAttrs(mWriter, tag->lowerName, state,
                          !(processFlags & hxProcessFlagAuto),
                          (processFlags & hxProcessFlagNeedsClose) != 0);
    }

    if (mWriter.activeLineCut() && (tag->is(HT_br) || tag->is(HT_wbr) || tag->is(HT_WBR)))
        mWriter.dropLine();

    return true;
}

/********************************************************/
bool HXSanitizer::closeTag(const NHtml::TTag* tag) {
    if (FilterEntities && FilterEntities->IsIgnoredTag(tag->lowerName)) {
        return true;
    }

    if (tag->is(HT_irreg)) {
        hxParseState state(nullptr, 0);
        unsigned processFlags = 0;
        doOpenTag(state, processFlags);
    }

    bool found = false;
    for (hxParseStack::reverse_iterator it = mTags.rbegin();
         it != mTags.rend(); ++it) {
        hxParseState& st = *it;

        if (st.mTag == tag) {
            found = true;
            break;
        }
        //Note: for legacy reason HT_subtxt has priority
        // over HT_subwin. 'HT_subwin|HT_subtxt' means just
        // the same as 'HT_subtxt'
        if ((st.mTag->is(HT_subwin) && !st.mTag->is(HT_irreg)) && !st.mTag->is(HT_subtxt))
            return false;
    }

    if (!found)
        return false;

    const NHtml::TTag* t;
    do {
        t = mTags.back().mTag;
        closeOneTag(tag);
    } while (t != tag);

    return true;
}

/********************************************************/
inline bool isSpaces(const char* s, unsigned len) {
    for (; len-- && *s; ++s) {
        if (strchr(" \t\r\n\f\v", *s) == nullptr) {
            return false;
        }
    }
    return true;
}
/********************************************************/
bool HXSanitizer::doText() {
    hxParseState state(nullptr, 0);
    unsigned processFlags = 0;

    if (isSpaces(mLexer.GetCurText(), mLexer.GetCurLength()))
        processFlags |= hxProcessFlagEmptyText;

    bool ret = false;

    if (doOpenTag(state, processFlags) && !state.is(hxIgnoreTag)) {
        const char* txt = mLexer.GetCurText();
        int len = mLexer.GetCurLength();

        if ((processFlags & hxProcessFlagEmptyText) != 0 && mTags.empty()) {
            //if (mLastEmptyOutput)
            //    return false;

            //txt = "\n";
            //len = 1;
        }
        onWrite(HTLEX_TEXT, processFlags);
        ret = mWriter.writeText(txt, len, state.mSpaceMode);
    }

    if (mOutTitle && state.is(hxInsideTitle)) {
        hxWriterForStroka wr(mCodePageIn, *mOutTitle);
        wr.writeText(mLexer.GetCurText(),
                     mLexer.GetCurLength(),
                     HX_SPACE_NORMAL);
    }

    return ret;
}

/********************************************************/
void HXSanitizer::writeTagWithAttrs(hxWriter& writer,
                                    const char* tagName,
                                    hxParseState& state,
                                    bool printAttributes,
                                    bool needsClose) {
    if ((++mCurDepth > mNestingDepth) && !needsClose) {
        return;
    }

    writer.writeSymbols("<", 1);
    writer.writeSymbols(tagName, strlen(tagName));
    mAttrSet.clear();
    for (int i = 0; printAttributes && i < mLexer.getCurNAtrs(); i++) {
        if (doAttribute(state, i, writer, nullptr, tagName) && MetaContainer) {
            MetaContainer->FlushContext();
        }
    }
    if (needsClose) {
        writer.writeSymbols(" /", 2);
        --mCurDepth;
    }
    writer.writeSymbols(">", 1);
}

/********************************************************/
bool HXSanitizer::doAttribute(hxParseState& state,
                              unsigned idx,
                              hxWriter& writer,
                              TString* altName,
                              const char* tagName) {
    const char* nameStart;
    const char* valStart;
    int nameLength;
    int valLength;
    if (!mLexer.getCurAttr(idx, nameStart, nameLength, valStart, valLength)) {
        return false;
    }
    TString nm(nameStart, 0, nameLength);
    nm.to_lower();

    if (mAttrSet.find(nm) != mAttrSet.end())
        return false;
    mAttrSet.insert(nm);

    if (nameStart == valStart)
        valStart = nm.data();
    nameStart = nm.data();

    if (nm == "/")
        return false;

    TString vl(valStart, 0, valLength);
    IMetaContainer::TContext* metaContext(nullptr);
    if (MetaContainer) {
        metaContext = MetaContainer->GetContext();
        metaContext->TagName = TString(tagName);
        metaContext->AttrName = nm;
        metaContext->AttrValue = vl;
    }

    TString vlconv;

    if ("class" == nm) {
        vlconv = mClassSanitizer->Sanitize(vl);
        if (!vlconv)
            return false;
        valStart = vlconv.data();
        valLength = vlconv.size();
    } else if ("style" == nm && mCssSanitizer && !!mcss_config_file) {
        mCssSanitizer->SetTagName(tagName);
        vlconv = vl;
        size_t vlLenBeforeDecode;
        do {
            vlLenBeforeDecode = vlconv.size();
            vlconv = HtEntDecodeToUtf(mCodePageIn, vlconv.data(), vlconv.size());
        } while (vlconv.size() != vlLenBeforeDecode);
        vlconv = mCssSanitizer->SanitizeInline(vlconv);
        if (!vlconv)
            return false;
        valStart = vlconv.data();
        valLength = vlconv.size();
    } else if (mChecker && !noSanitize()) {
        TString vlconvDecodedHref;
        if ("href" == nm) {
            vlconv = StripInPlace(vl);
            TString vlTmp = Strip(HtEntDecodeToUtf(mCodePageIn, vl.data(), vl.size()));
            for (size_t i = 0; i < vlTmp.size(); i++) {
                if (IsPrint(vlTmp[i]) && !IsSpace(vlTmp[i])) {
                    vlconvDecodedHref.push_back(vlTmp[i]);
                }
            }
        } else if ("src" == nm) {
            TString buf = TString::Uninitialized(vl.size() * 4);
            size_t written = 0;
            HtLinkDecode(vl.data(), buf.begin(), buf.size(), written, CharsetByName("CODES_UTF8"));
            vlconv = buf.substr(0, written);
        } else {
            vlconv = HtEntDecodeToUtf(mCodePageIn, vl.data(), vl.size());
        }
        bool res = false;
        if ("href" == nm) {
            size_t javascriptPosInHref = vlconvDecodedHref.find("javascript:");
            if (javascriptPosInHref == 0) {
                return false;
            }
        }
        if (!FilterEntities) {
            size_t javascriptPos = vlconv.find("javascript");
            if ("href" == nm) {
                if (javascriptPos == 0) {
                    return false;
                }
            } else {
                if (javascriptPos != TString::npos) {
                    return false;
                }
            }
        }
        if ((!strcmp(tagName, "a") || !strcmp(tagName, "area")) && (vlconv.find("data:") != TString::npos)) {
            return false;
        }

        if (FilterEntities && FilterEntities->AvoidChecker(vlconv)) {
            res = true;
        } else {
            valStart = vlconv.data();
            valLength = vlconv.size();
            mChecker->mCurState = &state;
            res = mChecker->check(nameStart, nameLength, valStart, valLength);
            mChecker->mCurState = nullptr;
        }

        if (!res)
            return false;
    }
    if (altName)
        *altName = nm;

    if (FilterEntities && (!FilterEntities->IsAcceptedAttr(tagName, nm.data()) || !FilterEntities->IsAcceptedValue(valStart, valLength))) {
        return true;
    }

    size_t offset = writer.getOffset() + getOffset() + 1;

    TString eTagName = tagName;
    eTagName.to_lower();
    TString eAttrName = nm.data();
    eAttrName.to_lower();
    TString eValStart = valStart;
    eValStart.to_lower();
    if (eTagName == "input" && eAttrName == "type" && eValStart == "password") {
        writer.writeSymbols(" autocomplete=\"new-password\"", 28);
    }

    writer.writeSymbols(" ", 1);
    writer.writeSymbols(nm.data(), nm.size());
    writer.writeSymbols("=\"", 2);

    TString attrValue(valStart, valLength);
    if (nameStart == valStart) {
        writer.writeSymbols(nm.data(), nm.size());
    } else {
        if (UrlProcessor && UrlProcessor->IsAcceptedAttr(tagName, nm.data())) {
            attrValue = UrlProcessor->Process(attrValue.data());
        } else if (FilterEntities) {
            FilterEntities->ProcessAnchor(tagName, nameStart, attrValue);
        }
        writer.writeText(attrValue.data(), attrValue.size(), HX_SPACE_ATTR);
    }

    writer.writeSymbols("\"", 1);

    if (MetaContainer) {
        metaContext->AttrPos = offset;
        metaContext->AttrLen = writer.getOffset() + getOffset() - offset;
        metaContext->AttrValue.erase();
        hxWriterForStroka wr(CODES_UTF8, metaContext->AttrValue);
        wr.writeText(attrValue.data(), attrValue.size(), HX_SPACE_ATTR);
    }

    return true;
}

/********************************************************/
bool HXSanitizer::closeOneTag(const NHtml::TTag* target) {
    bool ret = false;
    hxParseState& state = mTags.back();

    if (state.mTag->is(HT_br) || state.mTag->is(HT_wbr) || state.mTag->is(HT_WBR) || state.mTag->is(HT_par))
        mWriter.dropLine();

    if (!state.is(hxIgnoreContent)) {
        onWrite(HTLEX_END_TAG);

        if (--mCurDepth < mNestingDepth) {
            mWriter.writeSymbols("</", 2);
            mWriter.writeSymbols(state.mTag->lowerName, strlen(state.mTag->lowerName));
            mWriter.writeSymbols(">", 1);
        }
        ret = true;
    }

    if (inIM()) {
        if (state.mTag->is(HT_subwin) && !state.mTag->is(HT_irreg))
            popIMsubwin();
        if (state.mTag->is(HT_irreg)) {
            if (state.mTag == target)
                deleteFromIM(target);
            else
                onCloseIM(state.mTag);
        }
    }

    mTags.pop_back();
    (void)ret;
    //return ret?
    //return true;
    return ret;
}

/********************************************************/
void HXSanitizer::doBodyTag() {
    if (!mOutBodyTag)
        return;

    hxParseState st(nullptr, 0);

    mAttrSet.clear();
    for (int i = 0; i < mLexer.getCurNAtrs(); i++) {
        hxAttr a;

        {
            hxWriterForStroka wr(mCodePageIn, a.mPrint);
            if (!doAttribute(st, i, wr, &(a.mName), "body"))
                continue;
        }

        unsigned j;
        for (j = 0; j < mBodyAttrs->size(); j++) {
            if (a.isOf((*mBodyAttrs)[j])) {
                (*mBodyAttrs)[j] = a;
                break;
            }
        }
        if (j >= mBodyAttrs->size())
            mBodyAttrs->push_back(a);
    }
}

/********************************************************/
/********************************************************/
bool HXSanitizer::startIM() {
    freeIM();
    mIMStack = new hxIMStack();
    mIMStack->push_back(nullptr);
    return true;
}

/********************************************************/
bool HXSanitizer::freeIM() {
    if (!mIMStack)
        return false;

    while (mIMStack->size() > 0) {
        hxIMSet* s = mIMStack->back();
        if (s)
            delete s;
        mIMStack->pop_back();
    }
    delete mIMStack;
    mIMStack = nullptr;

    return true;
}

/********************************************************/
bool HXSanitizer::expandIM() {
    if (!inIM())
        return false;
    hxIMSet* s = mIMStack->back();
    if (!s || s->isReady())
        return false;
    bool ret = false;

    for (unsigned i = s->mOpened; i < s->mTags.size(); i++) {
        hxIMTag& t = (s->mTags)[i];

        hxParseState state(t.mTag, 0);
        unsigned processFlags = hxProcessFlagAuto;
        if (!doOpenTag(state, processFlags)) {
            myAssert(false);
            return false;
        }
        s->mOpened++;
        mTags.push_back(state);
        onWrite(HTLEX_START_TAG);
        mWriter.writeSymbols(t.mPrint.c_str(), t.mPrint.length());
        ret = true;
    }

    (void)ret;
    //return ret?
    return true;
}

/********************************************************/
bool HXSanitizer::collapseIM() {
    if (!inIM())
        return false;
    hxIMSet* s = mIMStack->back();
    if (!s)
        return false;

    bool ret = false;

    while (s->mOpened > 0) {
        closeOneTag();
        ret = true;
    }

    return ret;
}

/********************************************************/
bool HXSanitizer::pushIMsubwin() {
    if (!inIM())
        return false;
    if (!myAssert(!mIMStack->back() || mIMStack->back()->mOpened == 0)) {
        return false;
    }
    mIMStack->push_back(nullptr);
    return true;
}

/********************************************************/
bool HXSanitizer::popIMsubwin() {
    if (!inIM())
        return false;
    hxIMSet* s = mIMStack->back();
    if (!myAssert(!s || s->mOpened == 0))
        return false;
    mIMStack->pop_back();
    if (s)
        delete s;
    return true;
}

/********************************************************/
bool HXSanitizer::addToIM(hxIMTag& tag) {
    if (!inIM())
        return false;
    hxIMSet* s = mIMStack->back();
    if (!s) {
        s = new hxIMSet();
        mIMStack->back() = s;
    }
    if (!myAssert(s && s->isReady())) {
        return false;
    }
    s->mTags.push_back(tag);
    onWrite(HTLEX_START_TAG);
    mWriter.writeSymbols(tag.mPrint.c_str(), tag.mPrint.length());
    s->mOpened++;
    return true;
}

/********************************************************/
bool HXSanitizer::deleteFromIM(const NHtml::TTag* tag) {
    if (!inIM() || !tag)
        return false;
    hxIMSet* s = mIMStack->back();
    if (!myAssert(s && (s->mTags)[(s->mOpened) - 1].mTag == tag)) {
        return false;
    }
    hxIMTags::iterator it = s->mTags.begin();
    for (unsigned i = 1; i < s->mOpened; i++, it++) {
    }
    s->mTags.erase(it);
    s->mOpened--;
    return true;
}

/********************************************************/
bool HXSanitizer::onCloseIM(const NHtml::TTag* tag) {
    if (!inIM() || !tag)
        return false;
    hxIMSet* s = mIMStack->back();
    if (!myAssert(s && (s->mTags)[s->mOpened - 1].mTag == tag)) {
        return false;
    }
    s->mOpened--;
    return true;
}

/********************************************************/
/********************************************************/
// Specific logic of this sanitizer
/********************************************************/
const NHtml::TTag* HXSanitizer::identifyTag(const char* name, unsigned len, bool) {
    // const NHtml::TTag* ret = in_ht_tags_set(name, strlen(name));
    const NHtml::TTag* ret = &NHtml::FindTag(name, len);
    if (ret && (*ret) != HT_any)
        return ret;
    return nullptr;
}

/********************************************************/
bool HXSanitizer::doOpenTag(hxParseState& state,
                            unsigned& processFlags) {
    if (state.mTag && FilterEntities) {
        if (FilterEntities->IsIgnoredWithChildTag(state.mTag->lowerName)) {
            state.set(hxIgnoreWithChild);
            state.set(hxIgnoreContent);
            return true;
        } else if (FilterEntities->IsIgnoredTag(state.mTag->lowerName)) {
            return false;
        }
    }

    if (state.mTag && state.mTag->is(HT_empty))
        processFlags |= hxProcessFlagNeedsClose;

    if (state.mTag && *(state.mTag) == HT_BODY && mOutBodyTag)
        doBodyTag();

    while (true) {
        if (mTags.empty()) {
            state.mState = 0;
            state.mSpaceMode = HX_SPACE_NORMAL;
        } else {
            hxParseState& st = mTags.back();
            state.mState = st.mState;
            state.mSpaceMode = st.mSpaceMode;
        }

        if (state.mTag && state.mTag->is(HT_list))
            state.set(hxInsideLIST);

        if (state.mTag && *(state.mTag) == HT_TITLE && mOutTitle)
            state.set(hxInsideTitle);

        if (state.is(hxIgnoreContent)) {
            //ignore tag or text
            state.set(hxIgnoreTag);
            return true;
        } else {
            state.drop(hxIgnoreTag);
        }
        if (!state.mTag || !(state.mTag->is(HT_table) || state.mTag->is(HT_subtab))) {
            if (!(processFlags & hxProcessFlagEmptyText)) {
                // If tag or text is not inside table
                if (state.is(hxNeedsTableTD) && !state.is(hxInsideTD)) {
                    //open TD if required
                    openTag(sTagTD);
                    continue;
                }
                // Work with IM
                if (inIM() && !(processFlags & hxProcessFlagAuto)) {
                    if (!state.mTag ||
                        (!(state.mTag->is(HT_subwin) && !state.mTag->is(HT_irreg)) &&
                         !state.mTag->is(HT_subtxt)))
                    {
                        if (expandIM())
                            continue;
                    } else {
                        if (collapseIM())
                            continue;
                    }
                }
            }
        } else {
            //If tag is inside table
            if (!state.is(hxNeedsTableTD)) {
                //ignore it outside table
                state.set(hxIgnoreTag);
                return true;
            }
        }

        //TEXT
        if (!state.mTag)
            return true;

        // Check for dangerous tags with PCDATA inside
        if (state.mTag->is(HT_delete)) {
            state.set(hxIgnoreContent);
            return true;
        }

        // Close <P> if required
        if (state.mTag->is(HT_par)) {
            mWriter.dropLine();
            if (closeState(hxInsidePar))
                continue;
        } else {
            if (state.mTag->is(HT_br) || state.mTag->is(HT_wbr) || state.mTag->is(HT_WBR))
                mWriter.dropLine();
        }

        // Check ignored tags
        if (state.mTag->is(HT_ignore)) {
            state.set(hxIgnoreTag);
            return true;
        }

        bool needsSubWin = state.mTag->is(HT_subwin) && !state.mTag->is(HT_irreg);
        bool needsSubTxt = needsSubWin || state.mTag->is(HT_subtxt);

        switch (state.mTag->id()) {
            case HT_A:
                if (state.is(hxInsideA)) {
                    closeTag(sTagA);
                    continue;
                }
                state.set(hxInsideA);
                break;

            case HT_P:
                state.set(hxInsidePar);
                break;

            case HT_H1:
            case HT_H2:
            case HT_H3:
            case HT_H4:
            case HT_H5:
            case HT_H6:
                if (closeState(hxInsideH))
                    continue;
                state.set(hxInsideH);
                break;

            case HT_TABLE:
                state.drop(hxContentMask);
                state.set(hxNeedsTableTD);
                break;

            case HT_TBODY:
                if (state.is(hxInsideColGroup)) {
                    closeState(hxInsideColGroup);
                    continue;
                }
                if (closeState(hxInsideTable))
                    continue;
                state.set(hxInsideTable);
                break;
            case HT_TFOOT:
            case HT_THEAD:
                if (closeState(hxInsideTable))
                    continue;
                state.set(hxInsideTable);
                break;

            case HT_COLGROUP:
                if (closeState(hxInsideColGroup))
                    continue;
                state.set(hxInsideColGroup);
                break;

            case HT_TR:
                if (closeState(hxInsideTD))
                    continue;
                if (!state.is(hxInsideTable)) {
                    openTag(sTagTBODY);
                    continue;
                }
                if (closeState(hxInsideTR))
                    continue;
                state.set(hxInsideTR);
                break;

            case HT_TD:
            case HT_TH:
                if (closeState(hxInsideTD))
                    continue;
                if (!state.is(hxInsideTR)) {
                    openTag(sTagTR);
                    continue;
                }
                state.drop(hxContentMask);
                state.set(hxInsideTD);
                break;

            case HT_CAPTION:
                if (closeState(hxInsideTD))
                    continue;
                state.drop(hxContentMask);
                state.set(hxInsideTD);
                needsSubWin = needsSubTxt = true;
                break;

            case HT_DT:
            case HT_DD:
                if (closeState(hxInsideDD))
                    continue;
                state.drop(hxContentMask | hxNeedsTableTD);
                state.set(hxInsideDD);
                needsSubWin = needsSubTxt = true;
                break;

            case HT_FORM:
                if (closeState(hxInsideForm))
                    continue;
                state.set(hxInsideForm);
                break;

            case HT_ADDRESS:
                if (closeState(hxInsideAddress))
                    continue;
                state.set(hxInsideAddress);
                break;

            case HT_MARQUEE:
                if (closeState(hxInsideMarquee))
                    continue;
                state.set(hxInsideMarquee);
                break;

            case HT_OPTION:
                if (closeState(hxInsideOption))
                    continue;
                state.set(hxInsideOption);
                break;

            case HT_FIELDSET:
                state.set(hxInsideFieldset);
                break;

            case HT_LEGEND:
                if (!state.is(hxInsideFieldset)) {
                    openTag(sTagFIELDSET);
                    continue;
                }
                if (closeState(hxInsideLegend))
                    continue;
                state.set(hxInsideLegend);
                break;

            case HT_NOBR:
                if (mWriter.activeLineCut()) {
                    state.mSpaceMode = HX_SPACE_NOBR;
                    mWriter.clearNoBr();
                }
                break;

            case HT_PRE:
            case HT_LISTING:
            case HT_XMP:
            case HT_PLAINTEXT:
                state.mSpaceMode = HX_SPACE_PRE;
                break;

            case HT_TEXTAREA:
                state.mSpaceMode = HX_SPACE_IGNORE;
                break;

            case HT_LI:
                if (!state.is(hxInsideLIST)) {
                    openTag(sTagUL);
                    continue;
                }
                if (state.is(hxInsideLI)) {
                    closeTag(sTagLI);
                    continue;
                }
                state.set(hxInsideLI);
                break;

            default:

                if (needsSubTxt) {
                    state.drop(hxContentMask | hxNeedsTableTD);
                }
        }

        if (state.mTag->is(HT_lit) && state.mSpaceMode < HX_SPACE_IGNORE)
            state.mSpaceMode = HX_SPACE_LITERAL;

        if (mWriter.activeLineCut() && needsSubTxt && state.mSpaceMode < HX_SPACE_LITERAL)
            state.mSpaceMode = HX_SPACE_NORMAL;

        if (inIM() && needsSubWin)
            pushIMsubwin();

        break;
    }
    return true;
}

void HXSanitizer::onWrite(HTLEX_TYPE tp, unsigned processFlags) {
    mLastEmptyOutput = (tp == HTLEX_TEXT && mTags.empty() && (processFlags & hxProcessFlagEmptyText));
}

/********************************************************/
bool HXSanitizer::noSanitize() {
    return false;
}

/********************************************************/
/********************************************************/
HXSanitizerStream::HXSanitizerStream(NHtmlLexer::TObsoleteLexer& lexer,
                                     HXAttributeChecker* checker,
                                     std::ostream& out,
                                     ECharset cpIn,
                                     int lineLimit,
                                     Yandex::NCssSanitize::TClassSanitizer* classSanitizer,
                                     Yandex::NCssSanitize::TCssSanitizer* cssSanitizer,
                                     Yandex::NCssSanitize::TFormSanitizer* formSanitizer,
                                     TString css_config_file,
                                     int depth)
    : HXSanitizer(lexer, checker, cpIn, lineLimit, classSanitizer, cssSanitizer, formSanitizer, css_config_file, depth)
    , mOut(out)
{
}

/********************************************************/
HXSanitizerStream::~HXSanitizerStream() {
}

/********************************************************/
bool HXSanitizerStream::good() {
    return mOut.good();
}

/********************************************************/
bool HXSanitizerStream::writeOutput(const char* buf, unsigned size) {
    mLength += size;
    mOut.write(buf, size);
    return mOut.good();
}

/********************************************************/
/********************************************************/
