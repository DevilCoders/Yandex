#include "unpacker.h"

#include <kernel/snippets/archive/markup/markup.h>
#include <kernel/snippets/archive/view/storage.h>
#include <kernel/snippets/archive/view/order.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/archive/zone_checker/zone_checker.h>

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/iface/sentsfilter.h>

#include <kernel/snippets/iface/archive/segments.h>
#include <kernel/snippets/iface/archive/viewer.h>

#include <kernel/tarc/iface/dtiterate.h>

#include <library/cpp/charset/wide.h>

#include <util/generic/queue.h>
#include <util/generic/ptr.h>
#include <util/stream/mem.h>

namespace NSnippets {
    class TUnpacker::TImpl {
    private:
        struct THeapItem {
            typedef TSentsRangeList::TIterator TRef;
            TRef Beg;
            TRef End;
            THeapItem() {
            }
            THeapItem(const TRef& beg, const TRef& end)
                : Beg(beg)
                , End(end)
            {
            }
        };
        struct TGreaterFirst {
            bool operator()(const THeapItem& a, const THeapItem& b) const {
                return a.Beg->FirstId > b.Beg->FirstId;
            }
        };
        struct TGreaterLast {
            bool operator()(const THeapItem& a, const THeapItem& b) const {
                return a.Beg->LastId > b.Beg->LastId;
            }
        };
        struct TPoorAltsEraser {
            THolder<TForwardInZoneChecker> Alts;
            static TUtf16String DropAlts(const TSentParts& v, bool putSpaces) {
                TUtf16String res;
                for (const TSentPart& part : v) {
                    if (!part.InZone) {
                        res += part.Part;
                    } else if (putSpaces) {
                        res.append(part.Part.size(), wchar16(' '));
                    }
                }
                return res;
            }
            static void ProtectUsefulAlts(const TWtringBuf& cleanSent, TSentParts& v) {
                for (TSentPart& part : v) {
                    if (!cleanSent.Contains(part.Part)) {
                        part.InZone = false;
                    }
                }
            }
            void ErasePoorAlts(TArchiveSent& sent) {
                if (!Alts || !sent.Sent.size()) {
                    return;
                }

                TSentParts v = IntersectZonesAndSent(*Alts, sent, true);
                if (v.size() == 1 && !v[0].InZone) {
                    return;
                }

                const TUtf16String cleanSent = DropAlts(v, false);
                ProtectUsefulAlts(cleanSent, v);

                const TUtf16String res = DropAlts(v, true);
                sent.ReplaceSent(res);
            }
        };
        TUnpacker* Parent;
        const EARC SourceArc;
        typedef TVector<IArchiveViewer*> TViewers;
        typedef TPriorityQueue<THeapItem, TVector<THeapItem>, TGreaterFirst> TOpenQueue;
        typedef TPriorityQueue<THeapItem, TVector<THeapItem>, TGreaterLast> TCloseQueue;
        TViewers Viewers;
        TOpenQueue OpenQueue;
        TCloseQueue CloseQueue;

        int NextSent;
        TArchiveStorage* Storage;
        TArchiveMarkup* Markup;

        const ISentsFilter* SentsFilter;
        ui32 DocId;
        bool BeforeSents;
        TArchiveView UnpackedSents;
        TArchiveSentList::TIterator LastSent;
        ui32 HitsTopLength;
        bool AllPassageHits;
        THolder<TPoorAltsEraser> PoorAltsEraser;

    public:
        TImpl(TUnpacker* parent, const TConfig& cfg, TArchiveStorage* storage, TArchiveMarkup* markup, EARC sourceArc)
          : Parent(parent)
          , SourceArc(sourceArc)
          , OpenQueue()
          , CloseQueue()
          , NextSent(1)
          , Storage(storage)
          , Markup(markup)
          , SentsFilter(cfg.GetSentsFilter())
          , DocId(cfg.GetDocId())
          , BeforeSents(true)
          , UnpackedSents()
          , LastSent()
          , HitsTopLength(cfg.GetHitsTopLength())
          , AllPassageHits(cfg.IsAllPassageHits())
        {
            if (cfg.ErasePoorAlts()) {
                PoorAltsEraser.Reset(new TPoorAltsEraser());
            }
        }
        const TArchiveView& GetAllUnpacked() const {
            return UnpackedSents;
        }
        void AddRequest(TSentsOrder& order) {
            if (!order.Sents.Empty()) {
                OpenQueue.push(THeapItem(order.Sents.Begin(), order.Sents.End()));
            }
        }
        void AddRequester(IArchiveViewer* viewer) {
            Viewers.push_back(viewer);
            viewer->OnUnpacker(Parent);
        }

        //

        bool OnBeginExtendedBlock(const TArchiveTextBlockInfo& /*b*/) {
            return false;
        }
        bool OnEndExtendedBlock() {
            Y_ASSERT(false);
            return true;
        }
        bool OnMarkup(const void* markupInfo, size_t markupInfoLen);
        bool OnWeightZones(TMemoryInput* str) {
            for (TViewers::iterator i = Viewers.begin(); i != Viewers.end(); ++i) {
                (*i)->OnWeightZones(str);
            }
            return true;
        }
        bool OnHitBase(int hitBase) {
            for (TViewers::iterator i = Viewers.begin(); i != Viewers.end(); ++i) {
                (*i)->OnHitBase(hitBase);
            }
            return true;
        }
        int CanSkip() {
            if (BeforeSents) {
                BeforeSents = false;
                for (TViewers::iterator i = Viewers.begin(); i != Viewers.end(); ++i) {
                    (*i)->OnBeforeSents();
                }
            }
            if (!CloseQueue.empty()) {
                return 0;
            } else if (OpenQueue.empty()) {
                return -1;
            } else {
                return Max(0, OpenQueue.top().Beg->FirstId - NextSent);
            }
        }
        void Skip(int cnt) {
            NextSent += cnt;
        }
        void OnEnd() {
            while (!CloseQueue.empty()) {
                THeapItem x = CloseQueue.top();
                CloseQueue.pop();
                x.Beg->ResultEnd = LastSent;
                ++x.Beg;
                if (x.Beg != x.End) {
                    OpenQueue.push(x);
                }
            }
            while (!OpenQueue.empty()) {
                THeapItem x = OpenQueue.top();
                OpenQueue.pop();
                while (x.Beg != x.End) {
                    x.Beg->ResultBeg = x.Beg->ResultEnd = TArchiveSentList::TConstIterator();
                    ++x.Beg;
                }
            }
            for (TViewers::iterator i = Viewers.begin(); i != Viewers.end(); ++i) {
                (*i)->OnEnd();
            }
        }
        bool operator()(int sentId, const TUtf16String& rawSent, ui16 sentFlags) {
            if (BeforeSents) {
                BeforeSents = false;
                for (TViewers::iterator i = Viewers.begin(); i != Viewers.end(); ++i) {
                    (*i)->OnBeforeSents();
                }
            }

            if (SentsFilter == nullptr || SentsFilter->IsPermitted(DocId, sentId)) {
                LastSent = Storage->Add(SourceArc, sentId, rawSent, sentFlags);
                if (PoorAltsEraser) {
                    PoorAltsEraser->ErasePoorAlts(*LastSent);
                }
                UnpackedSents.PushBack(&*LastSent);

                while (!OpenQueue.empty() && OpenQueue.top().Beg->FirstId <= sentId) {
                    THeapItem x = OpenQueue.top();
                    OpenQueue.pop();
                    x.Beg->ResultBeg = LastSent;
                    CloseQueue.push(x);
                }
                while (!CloseQueue.empty() && CloseQueue.top().Beg->LastId <= sentId) {
                    THeapItem x = CloseQueue.top();
                    CloseQueue.pop();
                    x.Beg->ResultEnd = LastSent;
                    ++x.Beg;
                    if (x.Beg != x.End) {
                        OpenQueue.push(x);
                    }
                }
            }

            ++NextSent;
            return true;
        }

        //

        bool OnHeader(const TArchiveTextHeader* hdr) {
            if (SourceArc == ARC_LINK) {
                Markup->FilterHits(SourceArc, HitsTopLength, AllPassageHits);
                for (TViewers::iterator i = Viewers.begin(); i != Viewers.end(); ++i) {
                    (*i)->OnHitsAndSegments(Markup->GetFilteredHits(SourceArc), nullptr);
                }
            }
            return hdr && hdr->BlockCount;
        }

        bool OnMarkupInfo(const void* markupInfo, size_t markupInfoLen) {
            return OnMarkup(markupInfo, markupInfoLen) && OnHitBase(1);
        }

        bool OnBeginBlock(ui16 /*prevSentCount*/, const TArchiveTextBlockInfo& b) {
            int canSkip = CanSkip();
            if (canSkip == -1)
                return false;
            else if (canSkip >= b.SentCount) {
                Skip(b.SentCount);
                return false;
            }
            return true;
        }

        bool OnSent(size_t sentNum, ui16 sentFlag, const void* sentBytes, size_t sentBytesLen) {
            int canSkip = CanSkip();
            if (canSkip == -1) {
                return false;
            } else if (canSkip > 0) {
                Skip(1);
                return true;
            } else {
                TUtf16String sent;
                if (sentFlag & SENT_HAS_EXTSYMBOLS)
                    sent = TUtf16String((const wchar16*)sentBytes, sentBytesLen/sizeof(wchar16));
                else
                    sent = CharToWide((const char*)sentBytes, sentBytesLen, csYandex);

                if (!(*this)(sentNum, sent, sentFlag)) {
                    return false;
                }
            }
            return true;
        }

        bool OnEndBlock() {
            return true;
        }

    };

    bool TUnpacker::TImpl::OnMarkup(const void* markupInfo, size_t markupInfoLen) {
        Markup->SetArcMarkup(SourceArc, markupInfo, markupInfoLen);
        const TArchiveMarkupZones& mZones = Markup->GetArcMarkup(SourceArc);
        if (SourceArc == ARC_TEXT && PoorAltsEraser) {
            PoorAltsEraser->Alts.Reset(new TForwardInZoneChecker(mZones.GetZone(AZ_ALTERNATE).Spans, true));
        }
        if (SourceArc == ARC_TEXT && !Markup->GetSegments()) {
            Markup->ResetSegments(new NSegments::TSegmentsInfo(mZones));
            Markup->FilterHits(SourceArc, HitsTopLength, AllPassageHits);
            for (TViewers::iterator i = Viewers.begin(); i != Viewers.end(); ++i) {
                (*i)->OnHitsAndSegments(Markup->GetFilteredHits(SourceArc), Markup->GetSegments());
            }
        }
        for (TViewers::iterator i = Viewers.begin(); i != Viewers.end(); ++i) {
            (*i)->OnMarkup(mZones);
        }
        return true;
    }
    TUnpacker::TUnpacker(const TConfig& cfg, TArchiveStorage* storage, TArchiveMarkup* markup, EARC sourceArc)
      : Impl(new TImpl(this, cfg, storage, markup, sourceArc))
    {
    }
    TUnpacker::~TUnpacker() {
    }

    const TArchiveView& TUnpacker::GetAllUnpacked() const {
        return Impl->GetAllUnpacked();
    }
    void TUnpacker::AddRequest(TSentsOrder& order) {
        Impl->AddRequest(order);
    }
    void TUnpacker::AddRequester(IArchiveViewer* viewer) {
        Impl->AddRequester(viewer);
    }

    int TUnpacker::UnpText(const ui8* doctext) {
        IterateArchiveDocText(doctext, *Impl.Get());
        return 0;
    }

}
