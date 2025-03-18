#include "filter_entities.h"

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/digest/md5/md5.h>
#include <util/stream/str.h>
#include <util/system/yassert.h>

TFilterTags::TFilterTags(const TString& replaceTags, const TString& ignoreWithChildTags, const TString& ignoreTags, const TString& ignoreAttrs, const TString& badStyles, const TString& badMarkup, const TString& badAngBrText, const TString& badValue, bool doProcessAnchor)
    : DoProcessAnchor(doProcessAnchor)
{
    SetTagsForReplace(replaceTags);
    IgnoreWithChildTags.Compile(ignoreWithChildTags.data());
    Y_VERIFY(IgnoreWithChildTags.IsCompiled());
    IgnoreTags.Compile(ignoreTags.data());
    Y_VERIFY(IgnoreTags.IsCompiled());
    IgnoreAttrs.Compile(ignoreAttrs.data());
    Y_VERIFY(IgnoreAttrs.IsCompiled());
    BadStyles.Compile(badStyles.data());
    Y_VERIFY(BadStyles.IsCompiled());
    BadMarkup.Compile(badMarkup.data());
    Y_VERIFY(BadMarkup.IsCompiled());
    BadAngBrText.Compile(badAngBrText.data());
    Y_VERIFY(BadAngBrText.IsCompiled());
    BadValue.Compile(badValue.data());
    Y_VERIFY(BadValue.IsCompiled());
}

void TFilterTags::SetTagsForReplace(const TString& replaceTags) {
    typedef NJson::TJsonValue TJsonVal;
    typedef TJsonVal::TArray TJsonVec;

    if (replaceTags == "")
        return;
    TStringStream tagsForReplace;
    tagsForReplace << replaceTags;
    TJsonVal conf;
    Y_VERIFY(ReadJsonTree(&tagsForReplace, &conf), "incorrect json format");
    Y_VERIFY(conf.Has("replacements") && conf["replacements"].IsArray());
    const TJsonVec& replacements = conf["replacements"].GetArray();
    for (TJsonVec::const_iterator i = replacements.begin();
         i != replacements.end(); ++i) {
        Y_VERIFY(i->IsMap());
        Y_VERIFY(i->Has("before") && (*i)["before"].IsString());
        Y_VERIFY(i->Has("after") && (*i)["after"].IsString());
        TagsForReplace[(*i)["before"].GetString()] = (*i)["after"].GetString();
    }
}

bool TFilterTags::IsIgnoredWithChildTag(const char* tagName) {
    return IgnoreWithChildTags.GetRegExpr() && IgnoreWithChildTags.Match(tagName);
}

bool TFilterTags::IsIgnoredTag(const char* tagName) {
    return IgnoreTags.GetRegExpr() && IgnoreTags.Match(tagName);
}

bool TFilterTags::IsAcceptedAttr(const char* tagName, const char* attrName) {
    return !IgnoreAttrs.GetRegExpr() || !IgnoreAttrs.Match((TString(tagName) + "," + TString(attrName)).c_str());
}

bool TFilterTags::IsAcceptedAttrCSS(const char* /*tagName*/, const char* /*attrName*/) {
    return true;
}

bool TFilterTags::IsAcceptedStyle(const char* style_text) {
    TString style(style_text);
    style.to_lower();
    return (!BadStyles.GetRegExpr() || !BadStyles.Match(style.data()));
}

bool TFilterTags::IsAcceptedAngBrContent(const char* angbr_text) {
    TString text(angbr_text);
    text.to_lower();
    for (size_t i = 0; i != text.size(); ++i) {
        if (text[i] == '\n' || text[i] == '\r')
            text.replace(i, 1, " ");
    }
    return (!BadAngBrText.GetRegExpr() || !BadAngBrText.Match(text.data()));
}

bool TFilterTags::IsBadMarkup(const char* tag_text) {
    TString text(tag_text);
    text.to_lower();
    return (!!BadMarkup.GetRegExpr() && BadMarkup.Match(text.data()));
}

bool TFilterTags::IsAcceptedValue(const char* val, int len) {
    TString value(val, len);
    value.to_lower();
    return (!BadValue.GetRegExpr() || !BadValue.Match(value.data()));
}

void TFilterTags::ProcessAnchor(const char* tagName, const char* attrName, TString& attrValue) {
    if (!DoProcessAnchor)
        return;
    TStringStream val;
    if (!strcmp(attrName, "id") ||
        (*tagName == 'a' && !strcmp(attrName, "name"))) {
        val << '#';
    } else if (strcmp(attrName, "href") || *attrValue.data() != '#') {
        return;
    }
    val << attrValue << "7896" << Endl;
    char buf[33];
    attrValue += MD5::Stream(&val, buf);
}

TString TFilterTags::ReplaceTag(const char* tagName, int len) {
    TString tag(tagName, len);
    TMap<TString, TString>::const_iterator it = TagsForReplace.find(tag);
    if (it != TagsForReplace.end()) {
        return TString(it->second.data());
    }
    return tag;
}

bool TFilterTags::AvoidChecker(const TString& value) {
    return value.StartsWith("../message");
}
