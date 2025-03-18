#pragma once

#include <library/cpp/uri/http_url.h>

#include <library/cpp/charset/doccodes.h>
#include <util/generic/buffer.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <functional>

class IParsedDocProperties {
public:
    virtual ~IParsedDocProperties() {
    }

    // enumerate all values with given name.
    virtual int EnumerateValues(const char* name, std::function<void(const char*)> cb) const = 0;

    // returns the last value of the property
    virtual int GetProperty(const char* name, const char** prop) const = 0;

    /** @brief Obtains property value at the given index
    *   @param [in] name - The property name
    *   @param [in] index - The value index
    *   @returns The value if it is present or a null value otherwise
    */
    virtual TStringBuf GetValueAtIndex(const TStringBuf name, size_t index) const = 0;

    virtual bool HasProperty(const char* name) const = 0;

    virtual void Load(const TStringBuf& data) = 0;

    virtual TBuffer Serialize() const = 0;

    virtual int SetProperty(const char* name, const char* prop) = 0;

public:
    inline const THttpURL& GetBase() const {
        return BaseUrl;
    }

    inline ECharset GetCharset() const {
        return Charset;
    }

protected:
    // optimize frequent fields
    ECharset Charset;
    THttpURL BaseUrl;
};

THolder<IParsedDocProperties> CreateParsedDocProperties();

const char* GetUrl(IParsedDocProperties* docProps);
