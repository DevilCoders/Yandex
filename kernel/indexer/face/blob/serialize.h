#pragma once

#include <kernel/indexer/face/directtext.h>
#include <kernel/indexer/face/docinfo.h>
#include <kernel/indexer/direct_text/dt.h>

#include <util/generic/buffer.h>
#include <util/generic/strbuf.h>

namespace NIndexerCore {

/**
 * Reference to memory block with serialized direct-text entries.
 */
class TDirectTextRef : public TStringBuf {
public:
    inline explicit TDirectTextRef(const TBuffer& buf)
        : TStringBuf(buf.Data(), buf.Size())
    {
    }

    inline TDirectTextRef(const TStringBuf& buf)
        : TStringBuf(buf)
    {
    }
};

class TDirectTextHolder : public TNonCopyable, public TSimpleRefCount<TDirectTextHolder> {
public:
     TDirectTextHolder(const TDirectTextRef& data);
    ~TDirectTextHolder();

    const TDocInfoEx& GetDocInfo() const;

    const TDirectTextData2& GetDirectText() const;

    void FillDirectData(NIndexerCorePrivate::TDirectData* data, const TDirectTextData2* extra) const;

    void Process(IDirectTextCallback2& callback, IDocumentDataInserter* inserter,
        const TDirectTextData2* extraDirectText = nullptr) const;

private:
    class TImpl;
    THolder<TImpl> Impl_;
};

TBuffer SerializeDirectText(const TDocInfoEx& docInfo, const TDirectTextData2& data);

} // namespace NIndexerCore
