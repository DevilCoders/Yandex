#include "dump.h"
#include "pooldump.h"

#include <kernel/snippets/sent_match/sent_match.h>
#include <kernel/snippets/sent_match/glue.h>
#include <kernel/snippets/sent_match/tsnip.h>

#include <kernel/snippets/config/config.h>
#include <kernel/snippets/factors/factors.h>
#include <kernel/snippets/iface/archive/sent.h>
#include <kernel/snippets/sent_info/sent_info.h>
#include <kernel/snippets/strhl/hilite_mark.h>
#include <kernel/snippets/util/xml.h>

#include <util/generic/algorithm.h>
#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/tempbuf.h>
#include <utility>
#include <library/cpp/streams/bzip2/bzip2.h>
#include <util/stream/output.h>
#include <util/stream/buffer.h>
#include <util/stream/zlib.h>
#include <util/string/util.h>

namespace NSnippets
{
    class TDumpCallback::TImpl : public ISnippetCandidateDebugHandler {
    private:
        const TConfig& Cfg;
        TTops Tops;

    public:
        TImpl(const TConfig& cfg)
            : Cfg(cfg)
        {
        }

        IAlgoTop* AddTop(const char* algo, ECandidateSource source) override {
            if (Cfg.IsDumpForPoolGeneration() && source != CS_TEXT_ARC && source != CS_METADATA)
               return nullptr;

            Tops.push_back(TAlgoTop(algo));
            return &Tops.back();
        }
        void GetExplanation(IOutputStream& output) const;
    };

    TDumpCallback::TDumpCallback(const TConfig& cfg)
      : Impl(new TImpl(cfg))
    {
    }
    TDumpCallback::~TDumpCallback()
    {
    }
    ISnippetCandidateDebugHandler* TDumpCallback::GetCandidateHandler() {
        return Impl.Get();
    }

    void TDumpCallback::GetExplanation(IOutputStream& output) const {
        Impl->GetExplanation(output);
    }

    static TString ArcCoords(const TSnip& snip) {
        TArcFragments fragments = NSnipRedump::SnipToArcFragments(snip, false);
        return !!fragments.Fragments ? fragments.Save() : TString();
    }

    void TAlgoTop::Push(const TSnip& snip, const TUtf16String& title) {
        static const THiliteMark oc(u"<b>", u"</b>");
        if (snip.Factors.Size()) {
            TVector<TZonedString> tmp = snip.GlueToZonedVec(true);
            TVector<TString> text;
            for (size_t j = 0; j != tmp.size(); ++j) {
                tmp[j].Zones[+TZonedString::ZONE_MATCH].Mark = &oc;
                text.push_back(EncodeTextForXml10(TGluer::GlueToHtmlEscapedUTF8String(tmp[j]), false));
            }
            TSnipStat::TWordRanges wordRanges;
            for (const TSingleSnip& part : snip.Snips) {
                wordRanges.push_back(std::make_pair(part.GetFirstWord(), part.GetLastWord()));
            }
            Top.push_back(new TSnipStat(snip.Weight, snip.Factors, text, wordRanges, ArcCoords(snip), title));
        }
    }

  void TDumpCallback::TImpl::GetExplanation(IOutputStream& output) const
  {
     THolder<TBufferOutput> data;
     THolder<IOutputStream> outHold;
     IOutputStream* out;
     bool idx = true;
     bool weight = false;

     switch (Cfg.DumpCoding())
     {
         case 1:
             data.Reset(new TBufferOutput);
             outHold.Reset(new TZLibCompress(data.Get(), ZLib::GZip));
             out = outHold.Get();
             break;
         case 4:
             weight = true;
             [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
         case 3:
             idx = false;
             [[fallthrough]]; // AUTOGENERATED_FALLTHROUGH_FIXME
         case 2:
             out = &output;
             break;
         default:
             data.Reset(new TBufferOutput);
             outHold.Reset(new TBZipCompress(data.Get()));
             out = outHold.Get();
     };

     TPoolDumpCandidateChooser candChooser;
     if (Cfg.IsDumpForPoolGeneration()) {
        candChooser.Init(Tops);
     }

     for (TTops::const_iterator curTop = Tops.begin(); curTop != Tops.end(); ++curTop)
     {
         const TString& algoName = curTop->Name;
         TAlgoTop::TStorage snips = curTop->GetSortedSnips();

         for (int i = 0; i < snips.ysize(); ++i)
         {
            const TSnipStat& sstat = *snips[i];

            size_t taskId = 0;
            if (Cfg.IsDumpForPoolGeneration() && !(taskId = candChooser.GetTaskId(algoName, i))) {
                continue;
            }

            *out <<  Sprintf("%zu", sstat.TextForHtml.size()).data() << "\t";

            for (const TString& text : sstat.TextForHtml) {
                *out << text << "\t";
            }

            *out << (Cfg.IsDumpForPoolGeneration() ? ToString<size_t>(taskId) : algoName) << "\t";

            if (idx) {
                *out << i << "\t";
            }

            if (weight) {
                *out << sstat.SnipWeight << "\t";
            }

            for (const std::pair<int, int>& part : sstat.WordRanges) {
                *out << part.first << "-" << part.second << " ";
            }

            *out << "\t";
            for (TFactorDomain::TIterator it = sstat.Factors.GetDomain().Begin(); it.Valid(); it.Next()) {
                float factor = sstat.Factors[it.GetIndex()];
                if (!Cfg.IsDumpWithUnusedFactors() && (it.GetFactorInfo().IsUnusedFactor() || it.GetFactorInfo().IsDeprecatedFactor()) ||
                    !Cfg.IsDumpWithManualFactors() && it.GetFactorInfo().HasTagId(TFactorInfo::TG_SNIPPET_MANUAL))
                {
                    factor = 0.0;
                }
                *out << it.GetFactorInfo().GetFactorName() << ":" << factor << " ";
            }

            *out << "\t" << sstat.ArcCoords;
            *out << "\t" << WideToUTF8(sstat.Title);

            *out << "\n";
         }
     }
     out->Finish();

     if (!!data) {
         TString encoded = Base64Encode(TStringBuf(data->Buffer().Data(), data->Buffer().Size()));
         output.Write(encoded.data(), encoded.size());
     }
  }
}

