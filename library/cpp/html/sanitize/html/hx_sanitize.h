#pragma once

#include <util/system/defaults.h>
#include <util/generic/list.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>

#include <library/cpp/charset/codepage.h>
#include <library/cpp/uri/http_url.h>
#include <library/cpp/html/lexer/lex.h>
#include <library/cpp/html/spec/tags.h>
#include <library/cpp/html/sanitize/common/url_processor/url_processor.h>
#include <library/cpp/html/sanitize/common/filter_entities/filter_entities.h>
#include <util/generic/noncopyable.h>

namespace Yandex {
    namespace NCssSanitize {
        class TClassSanitizer;
        class TCssSanitizer;
        typedef TClassSanitizer TFormSanitizer;
    }
}

/********************************************************/
// Helpers for smart tag sanitizing
/********************************************************/

const unsigned hxInsideTitle = 0x01;
const unsigned hxIgnoreContent = 0x02;
const unsigned hxIgnoreTag = 0x04;
const unsigned hxNeedsTableTD = 0x08;
const unsigned hxInsideA = 0x10;
const unsigned hxInsideLIST = 0x20;
const unsigned hxIgnoreWithChild = 0x040;

const unsigned hxContentMask = 0xFFFF00;
const unsigned hxInsideAddress = 0x000100;
const unsigned hxInsideColGroup = 0x000200;
const unsigned hxInsideDD = 0x000400;
const unsigned hxInsideFieldset = 0x000800;
const unsigned hxInsideForm = 0x001000;
const unsigned hxInsideH = 0x002000;
const unsigned hxInsideLegend = 0x004000;
const unsigned hxInsideLI = 0x008000;
const unsigned hxInsideMarquee = 0x010000;
const unsigned hxInsidePar = 0x020000;
const unsigned hxInsideTD = 0x040000;
const unsigned hxInsideTR = 0x080000;
const unsigned hxInsideTable = 0x100000;
const unsigned hxInsideOption = 0x200000;

enum hxSpaceMode {
    // Normal mode: use <wbr> to cut long words
    //  words are separated by space symbols
    HX_SPACE_NORMAL,
    // Preformat mode: use <br> to cut long lines
    //  lines are separated by <br> and \n
    HX_SPACE_PRE,
    // Preformat mode: use <br> to cut long lines
    //  lines are separated by <br>, multiple space symbol
    //  sequences are equivalent to single space
    HX_SPACE_NOBR,
    // Literal mode: use \n to cut long lines
    //  lines are separated by \n
    HX_SPACE_LITERAL,

    // -- Technical internal modes ---
    // Ignore mode: no cutting at all
    HX_SPACE_IGNORE,
    // Attribute mode: special output for symbols + no cutting
    HX_SPACE_ATTR,
    // Output as it, no more transformations
    HX_SPACE_PLAIN
};

struct hxParseState {
    const NHtml::TTag* mTag;
    unsigned mState;
    hxSpaceMode mSpaceMode;

    bool is(unsigned flags) {
        return ((mState & flags) != 0);
    }

    void set(unsigned flag) {
        mState |= flag;
    }

    void drop(unsigned flag) {
        mState &= (~flag);
    }

    hxParseState(const NHtml::TTag* tag, unsigned state)
        : mTag(tag)
        , mState(state)
        , mSpaceMode(HX_SPACE_IGNORE)
    {
    }

    hxParseState(const hxParseState& st)
        : mTag(st.mTag)
        , mState(st.mState)
        , mSpaceMode(st.mSpaceMode)
    {
    }

    ~hxParseState() {
    }
};

using hxParseStack = TList<hxParseState>;

/********************************************************/
// Helper collection of attributes (body)

struct hxAttr {
    TString mName;
    TString mPrint;

    hxAttr()
        : mName()
        , mPrint()
    {
    }
    hxAttr(const hxAttr&) = default;
    hxAttr& operator=(const hxAttr&) = default;
    bool isOf(hxAttr& a) {
        return mName == a.mName;
    }
};

using hxAttrSet = TVector<hxAttr>;

/********************************************************/
// Helper for irregular markup

struct hxIMTag {
    const NHtml::TTag* mTag;
    TString mPrint;

    hxIMTag(const NHtml::TTag* tag)
        : mTag(tag)
        , mPrint(nullptr)
    {
    }

    hxIMTag(const hxIMTag&) = default;

    hxIMTag& operator=(const hxIMTag&) = default;

    bool isOf(const NHtml::TTag* tag) {
        return mTag == tag;
    }
};

using hxIMTags = TVector<hxIMTag>;

struct hxIMSet {
    hxIMTags mTags;
    unsigned mOpened;

    hxIMSet()
        : mTags()
        , mOpened(0)
    {
    }

    bool isReady() {
        return (mTags.size() == mOpened);
    }
};

using hxIMStack = TList<hxIMSet*>;

/********************************************************/
const int HX_MAX_ATTR_NAME_LENGTH = 40;

/********************************************************/
class HXAttributeChecker {
    friend class HXSanitizer;

protected:
    ECharset mCodePage;
    hxParseState* mCurState;
    THttpURL mBaseURL;
    bool resolveRelativeUrls;

    ECharset getCP() {
        return mCodePage;
    }

    hxParseState* getCurState() {
        return mCurState;
    }

    bool setBaseUrl(const char* baseURL);

    HXAttributeChecker(const char* baseURL);
    virtual ~HXAttributeChecker() {
    }

public:
    bool checkURL(const char*& valStart,
                  int& valLength,
                  TString& urlBufValue);

    virtual bool check(const char*& nameStart,
                       int& nameLength,
                       const char*& valStart,
                       int& valLength) = 0;
};

extern HXAttributeChecker* hxMakeAttributeChecker(const char* baseURL = nullptr);

/********************************************************/
const unsigned hxProcessFlagAuto = 0x1;
const unsigned hxProcessFlagNeedsClose = 0x2;
const unsigned hxProcessFlagEmptyText = 0x4;

/********************************************************/
class HXSanitizer;

class hxWriter {
protected:
    ECharset mCodePageIn;
    int mLineLimit;
    bool mNobrPrevSpace;

    unsigned char* mBuf;
    unsigned char* mBufEnd;
    unsigned char* mBufLast;
    unsigned char* mBufCur;
    unsigned char* mBufPoint;

    int mCurLineLength;
    int mPointLineLength;
    hxSpaceMode mPointSpaceMode;

    virtual void doWrite(const char* buf, int buflen) = 0;

    void writeBreak(hxSpaceMode mode);

    hxWriter(ECharset cp, int lineLimit);

public:
    virtual ~hxWriter();

    void dropLine();
    bool writeText(const char* buf,
                   size_t size,
                   hxSpaceMode spaceMode);
    void writeSymbols(const char* buf,
                      size_t buflen);

    void flush(bool complete = true);

    bool activeLineCut() {
        return (mLineLimit > 0);
    }

    void clearNoBr() {
        mNobrPrevSpace = false;
    }

    virtual size_t getOffset() {
        return 0;
    }

    char* getBuf() {
        return reinterpret_cast<char*>(mBuf);
    }
};

class hxWriterForStroka: public hxWriter, TNonCopyable {
protected:
    TString& mStroka;
    void doWrite(const char* buf, int buflen) override;

public:
    hxWriterForStroka(ECharset cp, TString& str);
    ~hxWriterForStroka() override;
    size_t getOffset() override;
};

class hxWriterForSanitizer: public hxWriter, TNonCopyable {
    friend class HXSanitizer;

protected:
    HXSanitizer& mSanitizer;
    void doWrite(const char* buf, int buflen) override;

public:
    hxWriterForSanitizer(ECharset cp,
                         HXSanitizer& sanitizer,
                         int line_limit);
    ~hxWriterForSanitizer() override;
};

/********************************************************/
class HXSanitizer : TNonCopyable {
    friend class hxWriterForSanitizer;

protected:
    static const NHtml::TTag* sTagA;
    static const NHtml::TTag* sTagFIELDSET;
    static const NHtml::TTag* sTagTBODY;
    static const NHtml::TTag* sTagTD;
    static const NHtml::TTag* sTagTR;
    static const NHtml::TTag* sTagLI;
    static const NHtml::TTag* sTagUL;

    NHtmlLexer::TObsoleteLexer& mLexer;
    HXAttributeChecker* mChecker;
    hxWriterForSanitizer mWriter;
    hxParseStack mTags;
    ECharset mCodePageIn;
    bool mUseBase;

    hxIMStack* mIMStack;

    hxAttrSet* mBodyAttrs;
    TString* mOutBodyTag;
    TString* mOutTitle;
    bool mLastEmptyOutput;
    bool mFailed;

    TSet<TString> mAttrSet;

    Yandex::NCssSanitize::TClassSanitizer* mClassSanitizer;
    bool ownClassSanitizer;

    Yandex::NCssSanitize::TCssSanitizer* mCssSanitizer;
    bool ownCssSanitizer;

    Yandex::NCssSanitize::TFormSanitizer* mFormSanitizer;

    TString mcss_config_file;

    int mNestingDepth;
    int mCurDepth;
    const char* mIgnoreTo;
    int mIgnoreDepth;
    size_t mLength;

    //----------------------------------------------
    // Main logic

    bool openTag(const NHtml::TTag* tag,
                 unsigned processFlags = hxProcessFlagAuto);
    bool closeTag(const NHtml::TTag* tag);
    bool doText();

    void writeTagWithAttrs(hxWriter& writer,
                           const char* tag_name,
                           hxParseState& state,
                           bool printAttributes,
                           bool needsClose);
    bool doAttribute(hxParseState& state,
                     unsigned idx,
                     hxWriter& writer,
                     TString* altName,
                     const char* tagName);

    //----------------------------------------------
    // Support

    bool closeOneTag(const NHtml::TTag* target = nullptr);
    void doBodyTag();

    unsigned curState() {
        return (mTags.empty()) ? 0 : mTags.back().mState;
    }

    bool closeState(unsigned stateFlag) {
        bool ret = false;
        while ((curState() & stateFlag) != 0) {
            closeOneTag();
            ret = true;
        }
        return ret;
    }

    const NHtml::TTag* getCurrentTag(bool isOpenTag);

    //----------------------------------------------
    // Irregular markup support

    bool inIM() {
        return (mIMStack != nullptr);
    }

    bool startIM();
    bool freeIM();

    bool expandIM();
    bool collapseIM();

    bool pushIMsubwin();
    bool popIMsubwin();

    bool addToIM(hxIMTag& tag);
    bool deleteFromIM(const NHtml::TTag* tag);

    bool onCloseIM(const NHtml::TTag* tag);

    //----------------------------------------------
    // This logic is available to modify in subclasses

    virtual bool writeOutput(const char* buf, unsigned size) = 0;

    virtual const NHtml::TTag* identifyTag(const char* name, unsigned len, bool onOpen);

    virtual bool doOpenTag(hxParseState& state,
                           unsigned& processFlags);
    /* work with text if state.mTag == 0 */

    virtual void onWrite(HTLEX_TYPE tp, unsigned processFlags = 0);

    virtual bool noSanitize();

    bool myAssert(bool condition);

public:
    HXSanitizer(NHtmlLexer::TObsoleteLexer& lexer,
                HXAttributeChecker* checker,
                ECharset cpIn = CODES_UTF8,
                int lineLimit = 0,
                Yandex::NCssSanitize::TClassSanitizer* classSanitizer = nullptr,
                Yandex::NCssSanitize::TCssSanitizer* cssSanitizer = nullptr,
                Yandex::NCssSanitize::TFormSanitizer* formSanitizer = nullptr,
                TString css_config_file = "",
                int depth = 180);

    virtual ~HXSanitizer();

    IUrlProcessor* UrlProcessor;
    IMetaContainer* MetaContainer;
    IFilterEntities* FilterEntities;

    bool isFailed() {
        return mFailed;
    }

    bool doIt(bool irregularMarkup = false,
              TString* outBodyTag = nullptr,
              TString* outTitle = nullptr);

    static void formDocument(std::istream& inProcessedDoc,
                             TString& inBodyTag,
                             TString& inTitle,
                             std::ostream& outDoc);

    int NestingDepth() {
        return mNestingDepth;
    }

    int CurDepth() {
        return mCurDepth;
    }

    size_t getOffset() {
        return mWriter.mBufCur - mWriter.mBuf + mLength;
    }
};

/********************************************************/
class HXSanitizerStream: public HXSanitizer {
protected:
    std::ostream& mOut;

    bool writeOutput(const char* buf, unsigned size) override;

public:
    HXSanitizerStream(NHtmlLexer::TObsoleteLexer& lexer,
                      HXAttributeChecker* checker,
                      std::ostream& out,
                      ECharset cpIn = CODES_UTF8,
                      int lineLimit = 0,
                      Yandex::NCssSanitize::TClassSanitizer* classSanitizer = nullptr,
                      Yandex::NCssSanitize::TCssSanitizer* cssSanitizer = nullptr,
                      Yandex::NCssSanitize::TFormSanitizer* formSanitizer = nullptr,
                      TString css_config_file = "",
                      int depth = 180);

    ~HXSanitizerStream() override;

    bool good();
};

/********************************************************/
TString HtEntDecodeToUtf(ECharset fromCharset, const char* str, size_t strLen);
