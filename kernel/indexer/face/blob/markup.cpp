#include "markup.h"

#include <kernel/indexer/face/blob/markup.pb.h>
#include <kernel/indexer/face/directtext.h>

#include <util/memory/pool.h>

namespace NIndexerCore {

///////////////////////////////////////////////////////////////////////////////

using namespace NIndexerCorePrivate;

///////////////////////////////////////////////////////////////////////////////

class TDirectMarkupHolder::TImpl {
public:
    TImpl()
        : Pool_(1 << 10)
    {
        UpdateDirectText();
    }

    ~TImpl() {
        for (auto mi = Markups_.begin(); mi != Markups_.end(); ++mi) {
            (*mi)->~TDirectMarkup();
        }
    }

    void Append(const TDirectMarkupRef& data) {
        TDirectMarkup* const markup = Pool_.New<TDirectMarkup>();
        //
        // Parse markup
        //
        if (markup->ParseFromArray(data.data(), data.size())) {
            Markups_.push_back(markup);
        } else {
            ythrow yexception() << "can't parse direct-markup from array";
        }
        //
        // Read attributes
        //
        for (auto ai = markup->attributes().begin(); ai != markup->attributes().end(); ++ai) {
            TDirectTextZoneAttr attr;
            TDirectAttrEntry* entries = Pool_.AllocateArray<TDirectAttrEntry>(ai->EntriesSize());

            attr.AttrName   = ai->GetName().c_str();
            attr.AttrType   = ai->GetType();
            attr.Entries    = entries;
            attr.EntryCount = ai->EntriesSize();

            for (size_t j = 0; j < attr.EntryCount; ++j) {
                entries[j].Sent = ai->GetEntries(j).GetPosition().GetSent();
                entries[j].Word = ai->GetEntries(j).GetPosition().GetWord();
                entries[j].AttrValue = (const wchar16*)ai->GetEntries(j).GetValue().c_str();
            }

            DirectData_.ZoneAttrs.push_back(attr);
        }
        //
        // Read zones
        //
        for (auto zi = markup->zones().begin(); zi != markup->zones().end(); ++zi) {
            TDirectTextZone zone;
            TZoneSpan* spans = Pool_.AllocateArray<TZoneSpan>(zi->SpansSize());

            zone.ZoneType  = zi->GetType();
            zone.Zone      = zi->GetName().c_str();
            zone.Spans     = spans;
            zone.SpanCount = zi->SpansSize();

            for (size_t j = 0; j < zone.SpanCount; ++j) {
                spans[j].SentBeg = zi->GetSpans(j).GetBegin().GetSent();
                spans[j].WordBeg = zi->GetSpans(j).GetBegin().GetWord();

                spans[j].SentEnd = zi->GetSpans(j).GetEnd().GetSent();
                spans[j].WordEnd = zi->GetSpans(j).GetEnd().GetWord();
            }

            DirectData_.Zones.push_back(zone);
        }
        // Need to do it every time because arrays can be reallocated.
        UpdateDirectText();
    }

    const TDirectTextData2* GetDirectText() const {
        return &DirectData_.DirectText;
    }

private:
    void UpdateDirectText() {
        DirectData_.DirectText.Entries = nullptr;
        DirectData_.DirectText.EntryCount = 0;
        DirectData_.DirectText.Zones = DirectData_.Zones.data();
        DirectData_.DirectText.ZoneCount = DirectData_.Zones.size();
        DirectData_.DirectText.ZoneAttrs = DirectData_.ZoneAttrs.data();
        DirectData_.DirectText.ZoneAttrCount = DirectData_.ZoneAttrs.size();
        DirectData_.DirectText.SentAttrs = nullptr;
        DirectData_.DirectText.SentAttrCount = 0;
    }

private:
    TDirectData DirectData_;
    TVector<TDirectMarkup*> Markups_;
    TMemoryPool Pool_;
};


TDirectMarkupHolder::TDirectMarkupHolder()
    : Impl_(new TImpl)
{
}

TDirectMarkupHolder::TDirectMarkupHolder(const TDirectMarkupRef& data)
    : Impl_(new TImpl)
{
    Impl_->Append(data);
}

TDirectMarkupHolder::~TDirectMarkupHolder()
{ }

void TDirectMarkupHolder::Append(const TDirectMarkupRef& data) {
    Impl_->Append(data);
}

const TDirectTextData2* TDirectMarkupHolder::GetDirectText() const {
    return Impl_->GetDirectText();
}

///////////////////////////////////////////////////////////////////////////////

} // namespace NIndexerCore
