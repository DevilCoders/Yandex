#pragma once

#include <kernel/indexer/direct_text/dt.h>

#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>

namespace NIndexerCore {

/** Pointer to serialized markup */
class TDirectMarkupRef : public TStringBuf {
public:
    inline explicit TDirectMarkupRef(const char* data, size_t len)
        : TStringBuf(data, len)
    {
    }
};

class TDirectMarkupHolder {
public:
    //! Create empty object
    TDirectMarkupHolder();

    //! Create object and append one element
    explicit TDirectMarkupHolder(const TDirectMarkupRef& data);
    ~TDirectMarkupHolder();

    void Append(const TDirectMarkupRef& data);

    const TDirectTextData2* GetDirectText() const;

private:
    class TImpl;
    THolder<TImpl> Impl_;
};

TBuffer SerializeMarkup(const TVector<TDirectTextZoneAttr>& attrs, const TVector<TDirectTextZone>& zones);

} // namespace namespace
