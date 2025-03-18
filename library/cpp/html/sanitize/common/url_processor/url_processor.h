#pragma once

class IUrlProcessor {
public:
    virtual ~IUrlProcessor(){};
    virtual TString Process(const char* sUrl) = 0;
    virtual bool IsAcceptedAttr(const char* tagName, const char* attrName) = 0;
};

class IMetaContainer {
public:
    struct TContext {
        TString TagName;
        TString AttrName;
        TString AttrValue;

        size_t AttrPos;
        size_t AttrLen;
        size_t AttrValPos;
        size_t AttrValLen;
    };

public:
    virtual ~IMetaContainer() {
    }
    virtual TContext* GetContext() = 0;
    virtual void FlushContext() = 0;
};
