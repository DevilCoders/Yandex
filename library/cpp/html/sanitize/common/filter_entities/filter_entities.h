#pragma once

#include <library/cpp/uri/http_url.h>

#include <library/cpp/regex/pcre/regexp.h>
#include <util/generic/map.h>

class IFilterEntities {
public:
    virtual ~IFilterEntities(){};
    virtual bool IsIgnoredWithChildTag(const char* tagName) = 0;
    virtual bool IsIgnoredTag(const char* tagName) = 0;
    virtual bool IsAcceptedAttr(const char* tagName, const char* attrName) = 0;
    virtual bool IsAcceptedAttrCSS(const char* tagName, const char* attrName) = 0;
    virtual bool IsAcceptedStyle(const char*) = 0;
    virtual bool IsAcceptedAngBrContent(const char*) = 0;
    virtual bool IsBadMarkup(const char*) = 0;
    virtual bool IsAcceptedValue(const char*, int) = 0;
    virtual void ProcessAnchor(const char* tagName, const char* attrName, TString& attrValue) = 0;
    virtual TString ReplaceTag(const char* tagName, int len) = 0;
    virtual bool AvoidChecker(const TString& value) = 0;
};

class TFilterTags: public IFilterEntities {
private:
    bool DoProcessAnchor;
    TRegExMatch IgnoreWithChildTags;
    TRegExMatch IgnoreTags;
    TRegExMatch IgnoreAttrs;
    TRegExMatch BadStyles;
    TRegExMatch BadAngBrText;
    TRegExMatch BadMarkup;
    TRegExMatch BadValue;
    TMap<TString, TString> TagsForReplace;

public:
    TFilterTags(const TString& replaceTags, const TString& ignoreWithChildTags, const TString& ignoreTags, const TString& ignoreAttrs, const TString& badStyles, const TString& badMarkup, const TString& badAngBrText, const TString& badValue, bool doProcessAnchor = true);
    void SetTagsForReplace(const TString& ReplaceTags);
    bool IsIgnoredWithChildTag(const char* tagName) override;
    bool IsIgnoredTag(const char* tagName) override;
    bool IsAcceptedAttr(const char* tagName, const char* attrName) override;
    bool IsAcceptedAttrCSS(const char* tagName, const char* attrName) override;
    bool IsAcceptedStyle(const char*) override;
    bool IsAcceptedAngBrContent(const char*) override;
    bool IsBadMarkup(const char*) override;
    bool IsAcceptedValue(const char*, int) override;
    void ProcessAnchor(const char* tagName, const char* attrName, TString& attrValue) override;
    TString ReplaceTag(const char* tagName, int len) override;
    bool AvoidChecker(const TString& value) override;
};
