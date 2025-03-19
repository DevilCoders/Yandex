#include "orangeattrs.h"

#include <ysite/yandex/erf_format/erf_format.h>

#include <util/string/cast.h>
#include <util/string/type.h>
#include <util/generic/yexception.h> // for Y_ENSURE

ui32 TimestampFromFeed(const char* val) {
    TString r = ParamFromAttrValue(val, "date");

    if (!!r)
        return FromString<ui32>(r);

    r = ParamFromAttrValue(val, "time");

    if (!!r)
        return FromString<ui32>(r);

    return 0;
}

inline ui32 TimestampFromFeedAttrs(const TErfAttrs& hash, const TString& name) {
    TErfAttrs::const_iterator it = hash.find(name);
    if (it == hash.end())
        return 0;
    return TimestampFromFeed(it->second.c_str());
}

inline ui32 TimestampFromFeedAttrsNoThrow(const TErfAttrs& hash, const TString& name) {
    try {
        return TimestampFromFeedAttrs(hash, name);
    }
    catch (TFromStringException&) {
        return 0;
    }
}

static inline ui8 MapVisitRate(double rate) {
    double clamped = ClampVal(rate, 0.01, 1000.0);
    double rateLog = log(clamped);
    double min = log(0.01);
    double max = log(1000.0);
    return ClampVal<ui8>(ui8((rateLog - min) / (max - min) * 256), 0, 255);
}

static bool UpdateErfTimestamp(SDocErfInfo3& erf, ui32 timestamp, ui32 from) {
    if (timestamp == 0) {
        return false;
    }
    if (erf.Timestamp == 0 || timestamp < erf.Timestamp) {
        erf.Timestamp = timestamp;
        erf.TimestampFrom = from;
        return true;
    }
    return  false;
}

void FillErfTimeCrawlRankVisitRate(const TErfAttrs& attrs, SDocErfInfo3& erf)
{

    try {

        UpdateErfTimestamp(erf, TimestampFromFeedAttrs(attrs, "news"), TF_FROM_NEWS_FEED);
        UpdateErfTimestamp(erf, TimestampFromFeedAttrs(attrs, "post"), TF_FROM_POSTS_FEED);// temporaly decision (errors in feed descriptions)
        UpdateErfTimestamp(erf, TimestampFromFeedAttrs(attrs, "posts"), TF_FROM_POSTS_FEED);
        UpdateErfTimestamp(erf, TimestampFromFeedAttrsNoThrow(attrs, "blogs"), TF_FROM_BLOGS_FEED);
        UpdateErfTimestamp(erf, TimestampFromFeedAttrs(attrs, "trusted_blogs"), TF_FROM_TRUSTED_BLOGS);

        // fill Timestamp from orange and yabar at last, if haven't any aothers Timestamps
        if (!erf.Timestamp) {
            UpdateErfTimestamp(erf, TimestampFromFeedAttrs(attrs, "orange"), TF_FROM_ORANGE);
            UpdateErfTimestamp(erf, TimestampFromFeedAttrs(attrs, "yabar"), TF_FROM_BAR);
        }

        {
            TString value = ParamFromArcHeader(attrs, "posts", "ft");
            if ("forum" == value)
                erf.PostFrom = PF_FROM_FORUM;
            else if ("blog" == value)
                erf.PostFrom = PF_FROM_BLOG;
        }
        {
            TString value = ParamFromArcHeader(attrs, "posts", "yarss");
            if (IsTrue(value))// IsTrue won't return true on empty strings anymore
                erf.PostYarss = 1;
        }
        {
            TString value = ParamFromArcHeader(attrs, "orange", "CrawlRankErf");
            if (!!value)
                erf.CrawlRank = (ui8) FromString<ui32>(value);
        }
        {
            TString value = ParamFromArcHeader(attrs, "yabar", "visitRate");
            if (!!value)
                erf.YabarFastVisitRate = MapVisitRate(FromString<double>(value));
        }

    } catch (yexception) {
        ythrow yexception() << "error during processing : " << CurrentExceptionMessage();
    }

}

static bool ParseAnnotationLine(const TString& annLine, std::pair<TString, TString>& res) {
    TString snippet = annLine;
    // until first tab, or all snippet
    size_t i = snippet.find('\t');
    if (i != TString::npos)
        snippet = snippet.substr(0, i);

    // to insert into text attrs, we need set name for fields
    // name is part of snippet before "="
    i = snippet.find('=', 0);
    if (i != TString::npos && (i + 1) < snippet.size()) {
        res.first = snippet.substr(0, i);
        res.second = snippet.substr(i + 1);
    } else {
        return false;
    }
    return true;
}

TAnnotationProcessor::TSnippetsParser::TSnippetsParser(const TString& preferredSource)
    : PreferredSource(preferredSource)
{}

void TAnnotationProcessor::TSnippetsParser::Init() {
    StoredSnippet.first.clear();
    StoredSnippet.second.clear();
    LastSnippetModTime = 0;
    HadPreferredSource = false;
    BadSnippetsNum = 0;
}

bool TAnnotationProcessor::TSnippetsParser::ProcessAnnotationAttrs(const TString& source, const TString& /*type*/, const ui32 modTime) {
    bool nowIsPrefered = source == PreferredSource;

    // skip values if had PrefferedSource but now not preffered
    if (HadPreferredSource && !nowIsPrefered)
        return false;

    if (!HadPreferredSource)
        HadPreferredSource = nowIsPrefered;

    ModTime = modTime;

    bool needParseValues = nowIsPrefered || ModTime > LastSnippetModTime || StoredSnippet.first.empty();

    // return true if need parse values
    return needParseValues;
}

void TAnnotationProcessor::TSnippetsParser::ProcessAnnotationValue(const TString& annotationValue) {

    std::pair<TString, TString> nameValue;
    if (ParseAnnotationLine(annotationValue, nameValue)) {
        // TODO can be multiple line for snippets now only last line will bw stored
        StoredSnippet = nameValue;
        LastSnippetModTime = ModTime;
    } else {
        ++BadSnippetsNum;
        // if has errror in ParseAnnotationLine and StoredSnippet not filled clear HasPreferredSource and LastSnippetModTime
        if (StoredSnippet.first.empty()) {
            HadPreferredSource = false;
            LastSnippetModTime = 0;
        }
    }
}

void TAnnotationProcessor::TSnippetsParser::SetSnippet(TFullDocAttrs& docAttrs) {
    if (!StoredSnippet.first.empty())
        docAttrs.AddAttr(StoredSnippet.first, StoredSnippet.second, TFullDocAttrs::AttrArcText);
}

TAnnotationProcessor::TUrlHashParser::TUrlHashParser(bool useErfcreator)
    : UseErfCreator(useErfcreator)
{}

void TAnnotationProcessor::TUrlHashParser::Init() {
    UniqueHashes.clear();
    BadUrlHashesNum = 0;
}

bool TAnnotationProcessor::TUrlHashParser::ProcessAnnotationAttrs(const TString& /*source*/, const TString& /*type*/, const ui32 /*modTime*/) {
    return true;
}

void TAnnotationProcessor::TUrlHashParser::ProcessAnnotationValue(const TString& annotationValue) {
    try {
        UniqueHashes.insert(FromString<ui64>(annotationValue));
    } catch (yexception& /* ex */) {
        ++BadUrlHashesNum;
    }
}

void TAnnotationProcessor::TUrlHashParser::SetUniqueHashes(const ui64 erfUrlHash, NRealTime::TIndexedDoc& indexedDoc) const {
    for (THashSet<ui64>::const_iterator i = UniqueHashes.begin(); i != UniqueHashes.end(); ++i) {
        const ui64& urlHash = *i;
        // put all url hashes if not use erfCreator
        if (urlHash != erfUrlHash || !UseErfCreator)
            indexedDoc.AddUrlHashes(*i);
    }
}

inline bool IsOtherDelimiter(TString::char_type ch) {
    return (ch == ';' || ch == ',' || ch == '=');
}

bool GetPart(const TString& line, TString::char_type delimiter, size_t& startIndex, TString* res) {
    size_t i = startIndex;
    size_t lineLen = line.size();
    bool inQuotes = false;
    bool isEscaped = false;

    for ( ; i < +lineLen; ++i) {
        TString::char_type ch = line[i];
        if (isEscaped) {
            // unescape \" and \t in any cases
            if (ch == '\"')
                res->append(ch);
            else if (ch == 't')
                res->append('\t');
            else if (inQuotes) { // in quotes dont unescape anything
                res->append('\\');
                res->append(ch);
            } else
                res->append(ch);

            isEscaped = false;
            continue;
        }
        // define escaped symbol
        if (ch == '\\') {
            isEscaped = true;
            continue;
        }

        // appears not escaped quote, get all symbols in quotes
        if (ch == '\"') {
            inQuotes = !inQuotes;
            continue;
        }

        if (!inQuotes) {
            if (ch == delimiter) {
                startIndex = i + 1;
                return true;
            } else if (IsOtherDelimiter(ch)) {
                // exit with error if presents other delimiters without backslash
                return false;
            }
        }
        res->append(ch);
    }
    startIndex = i;
    return (!inQuotes && res->length());
}

bool ParseNavigationalLine(const TString& line, NRobot::TNavInfo& navInfo) {
    const size_t lineLen = line.size();

    for (size_t i = 0; i < +lineLen; ) {
        TString names[2];
        TString values[2];
        if (!GetPart(line, '=', i, &names[0]))
            return false;

        if (!GetPart(line, ',', i, &values[0]))
            return false;

        if (!GetPart(line, '=', i, &names[1]))
            return false;

        if (!GetPart(line, ';', i, &values[1]))
            return false;

        NRobot::TNavInfo::TNavItem item;
        for (size_t j = 0; j < Y_ARRAY_SIZE(names); ++j) {
            if (names[j] == "query") {
                item.SetQuery(values[j]);
            } else if (names[j] == "adhoc")
                item.SetAdHoc(values[j]);
            else
                return false;
        }

        if (item.HasQuery() && item.HasAdHoc()) {
            NRobot::TNavInfo::TNavItem& addedItem = *navInfo.AddNavItems();
            addedItem.CopyFrom(item);
        } else
            return false;   // any bad annotations are errors
    }

    return true;
}

void TAnnotationProcessor::TNavigationalParser::Init() {
    BadNavigationalsNum = 0;
    NavInfo.Clear();
}

bool TAnnotationProcessor::TNavigationalParser::ProcessAnnotationAttrs(const TString& /*source*/, const TString& /*type*/, const ui32 /*modTime*/) {
    return true;
}

void TAnnotationProcessor::TNavigationalParser::ProcessAnnotationValue(const TString& annotationValue) {
    if (!ParseNavigationalLine(annotationValue, NavInfo)) {
        ++BadNavigationalsNum;
    }
}

void TAnnotationProcessor::TNavigationalParser::SetNavigationalInfo(NRobot::TNavInfo& navInfo) {
    if (NavInfo.NavItemsSize())
        navInfo.MergeFrom(NavInfo);
}

void TAnnotationProcessor::TNewsParser::Init() {
    LastModTime = 0;
    CurrentModTime = 0;
    NewsOrangeAnnotations.Clear();
}

bool TAnnotationProcessor::TNewsParser::ProcessAnnotationAttrs(const TString& /*source*/, const TString& /*type*/, const ui32 modTime) {
    CurrentModTime = modTime;
    return true;
}

void TAnnotationProcessor::TNewsParser::ProcessAnnotationValue(const TString& annotationValue) {
    // Use only last annotation
    if (CurrentModTime <= LastModTime) {
        return;
    }

    LastModTime = CurrentModTime;
    NewsOrangeAnnotations.Parse(annotationValue);
}

bool TAnnotationProcessor::TNewsParser::UpdateLinks(TAnchorText* anchorText) {
    // skip empty anchorText
    Y_ENSURE(anchorText, "Zero anchorText pointer");

    typedef TAnchorText_TAnchorTextEntry TLinkEntry;
    typedef ::google::protobuf::RepeatedPtrField<TLinkEntry> TLinkEntries;
    TLinkEntries* linkEntries = anchorText->MutableAnchorText();

    // skip all web links
    linkEntries->Clear();
    for(size_t i = 0; i < NewsOrangeAnnotations.Urls.size(); ++i) {
        TLinkEntry& linkEntry = *linkEntries->Add();
        const std::pair<TString, TString>& linkData = NewsOrangeAnnotations.Urls[i];

        linkEntry.SetUrl(linkData.first);
        linkEntry.SetAnchorText(linkData.second);
    }
    return true;
}

TAnnotationProcessor::TAnnotationProcessor(const TString& prefferedSource, bool useErfCreator)
    : SnippetsParser(prefferedSource)
    , UrlHashParser(useErfCreator)
{
}

void TAnnotationProcessor::Process(
    const google::protobuf::RepeatedPtrField<NOrangeData::TDocAnnotation>& annotations,
    bool useNewsAnnotationParser,
    TFullDocAttrs& docAttrs,
    NRealTime::TIndexedDoc& indexedDoc,
    TAnchorText* anchorText)
{
    SnippetsParser.Init();
    UrlHashParser.Init();
    NavigationalParser.Init();
    NewsParser.Init();

    for (TAnnotationIterator it = annotations.begin(); it != annotations.end(); ++it) {
        const TString& annType = it->GetType();
        IAnnotationParser* parser = nullptr;
        if (annType == "snippets") {
            parser = &SnippetsParser;
        } else if (annType == "urlhash") {
            parser = &UrlHashParser;
        } else if (annType == "navigational") {
            parser = &NavigationalParser;
        } else if (useNewsAnnotationParser && annType == "news") {
            parser = &NewsParser;
        } else {
            continue;
        }

        if (parser->ProcessAnnotationAttrs(it->GetSource(), it->GetType(), it->GetModTime())) {
            const TValues& annotationValues = it->GetValues();
            for (int i = 0; i < annotationValues.size(); ++i)
                parser->ProcessAnnotationValue(annotationValues.Get(i));
        }
    }

    SnippetsParser.SetSnippet(docAttrs);
    NavigationalParser.SetNavigationalInfo(*indexedDoc.MutableNavInfoUtf());
    if (useNewsAnnotationParser)
        NewsParser.UpdateLinks(anchorText);
}

void TAnnotationProcessor::SetUniqueHashes(const ui64 erfUrlHash, NRealTime::TIndexedDoc& indexedDoc) const {
    UrlHashParser.SetUniqueHashes(erfUrlHash, indexedDoc);
}
