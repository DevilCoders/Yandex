#include <tools/segutils/segcommon/data_utils.h>
#include <tools/segutils/segcommon/qutils.h>

#include <kernel/segmentator/structs/structs.h>
#include <kernel/segnumerator/segnumerator.h>
#include <kernel/segutils/numerator_utils.h>

#include <library/cpp/getopt/opt.h>

#include <util/generic/utility.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

using namespace NSegm;

namespace NSegutils {

enum ELinksMode {
    LM_NONE = 0, LM_BLOCK, LM_CONTEXT,
};

typedef TVector<ui16> TLinkSents;

bool IsPositive(ELinksMode mode, const NSegm::TSegmentSpans::const_iterator& it) {
    using namespace NSegm;
    return LM_BLOCK == mode && (STP_AUX == it->Type || STP_LINKS == it->Type || STP_FOOTER
            == it->Type) || LM_CONTEXT == mode && (STP_CONTENT == it->Type);
}

template<typename TParent>
class TLinkSentsHandler: public TParent {
    enum EState {
        State_Idle, State_Anchor, State_Sape, State_External, State_Added
    };
    TLinkSents Sents;
    EState State;
    ELinksMode Mode;

public:
    TLinkSentsHandler() :
        TParent(), State(State_Idle), Mode(LM_NONE) {
    }

    void SetMode(ELinksMode mode) {
        Mode = mode;
    }

    void CheckSegment(TStat& stat, TLinkSents::const_iterator& sit, const TSegmentSpans::const_iterator& it) const {
        while (sit != Sents.end() && *sit < it->Begin.Sent()) {
            stat.AddFn();
            ++sit;
        }

        if (sit != Sents.end() && !it->ContainsSent(*sit)) {
            if (IsPositive(Mode, it)) {
                stat.AddFp(it->Links - it->LocalLinks);
            }
        }

        while (sit != Sents.end() && it->ContainsSent(*sit)) {
            if (IsPositive(Mode, it)) {
                stat.AddTp();
            } else {
                stat.AddFn();
            }
            ++sit;
        }
    }

    bool OnMoveInputImpl(const THtmlChunk& chunk, const TZoneEntry& ze, const TNumerStat& stat) {
        bool res = TParent::OnMoveInputImpl(chunk, ze, stat);

            if (ze.IsOpen) {
               Y_ASSERT(ze.Name);
               State = chunk.Tag && chunk.Tag->id() == HT_A ? State_Anchor : State = State_Idle;
            }
            for (size_t i = 0; i < ze.Attrs.size(); ++i) {
                 TAttrEntry attr = ze.Attrs[i];
                 TString attrValue = attr.Value;
                 attrValue.to_lower();
                 if (!strcmp("_s_class", ~attr.Name) && attrValue == "sape") {
                     if (State_External == State) {
                         Sents.push_back(stat.TokenPos.Break());
                         State = State_Added;
                     } else if (State_Anchor == State)
                         State = State_Sape;
                 } else if (!strcmp("link", ~attr.Name)) {
                     if (State_Sape == State || LM_CONTEXT == Mode) {
                         Sents.push_back(stat.TokenPos.Break());
                         State = State_Added;
                      } else if (State_Anchor == State)
                         State = State_External;
                 }
            }
        return res;
        /*
        const THtmlChunk* htev = ev.Chunk;

        switch (htev->flags.type) {
        case PARSED_ZONE_OPEN:
            State = htev->Tag && htev->Tag->id() == HT_A ? State_Anchor : State = State_Idle;
            break;
        case PARSED_ATTRIBUTE: {
            TString s(htev->text, htev->leng);
            s.to_lower();
            if (!strcmp("_s_class", htev->name) && s == "sape") {
                if (State_External == State) {
                    Sents.push_back(stat.TokenPos.Break());
                    State = State_Added;
                } else if (State_Anchor == State)
                    State = State_Sape;
            } else if (!strcmp("link", htev->name)) {
                if (State_Sape == State || LM_CONTEXT == Mode) {
                    Sents.push_back(stat.TokenPos.Break());
                    State = State_Added;
                } else if (State_Anchor == State)
                    State = State_External;
            }

            break;
        } default:
            State = State_Idle;
        }

        return res;*/
    }

    TStat CheckLinks() const {
        using namespace NSegm;
        TStat stat;
        TSegmentSpans spans = TParent::GetSegmentSpans();
        TLinkSents::const_iterator sit = Sents.begin();

        for (TSegmentSpans::const_iterator it = spans.begin(); it != spans.end(); ++it) {
            CheckSegment(stat, sit, it);
        }

        if (Sents.size() > stat.Tp() + stat.Fn())
            ythrow yexception () << Sents.size() << " > " << (stat.Tp() + stat.Fn());

        return stat;
    }
};

typedef TLinkSentsHandler< NSegm::TSegmentatorHandler<> > TNumHandler;

TStat ProcessDoc(const THtmlDocument& data, TParserContext& ctx, ELinksMode mode) {
    TNumHandler handler;
    handler.SetMode(mode);

    try {
        ctx.SegNumerateDocument(&handler, data);
        TStat newstat, oldstat;

        try {
            newstat = handler.CheckLinks();
        } catch (const yexception& e) {
            Clog << "new: " << e.what() << Endl;
        }

        return newstat;
    } catch (const yexception& e) {
        Clog << e.what() << Endl;
        return TStat();
    }
}

void ProcessStats(const TStats& stats) {
    ui32 n = 0;
    float dpr = 0;
    float dre = 0;
    ui32 tp = 0;
    ui32 fn = 0;
    ui32 fp = 0;
    ui64 tot = 0;

    for (TStats::const_iterator it = stats.begin(); it != stats.end(); ++it) {
        if (*it) {
            ++n;
            dpr += it->Pr();
            dre += it->Re();
            tp += it->Tp();
            fn += it->Fn();
            fp += it->Fp();
            tot += it->Total();
        }
    }

    dpr = n ? dpr / n : 1;
    dre = n ? dre / n : 1;
    float lre = (tp + fn) ? float(tp) / (tp + fn) : 1;
    float lpr = (tp + fp) ? float(tp) / (tp + fp) : 1;

    Cout << "Documents: " << n << Endl;
    Cout << "Links: " << tot << Endl;
    Cout << "Average pr on a document: " << dpr << Endl;
    Cout << "Average re on a document: " << dre << Endl;
    Cout << "F1 on averages: " << (dpr && dre ? 2 * dpr * dre / (dpr + dre) : 0) << Endl;
    Cout << "Total pr: " << lpr << Endl;
    Cout << "Total re: " << lre << Endl;
    Cout << "F1 on totals: " << (lpr && lre ? 2 * lpr * lre / (lpr + lre) : 0) << Endl;
}

void Usage(const char * me) {
    Cerr << me << " -c <config_dir> -f <filesDir> -l <b|c> [-n <max>]" << Endl;
}

bool GetOpts(int argc, const char** argv, TString& configDir, TString& filesDir, ELinksMode& mode,
        ui32& max) {
    Opt opts(argc, argv, "c:f:l:n:");
    int optlet;
    while (EOF != (optlet = opts.Get())) {
        switch (optlet) {
        default:
            Usage(argv[0]);
            return false;
        case 'c':
            configDir = opts.GetArg();
            break;
        case 'f':
            filesDir = opts.GetArg();
            break;
        case 'n':
            max = FromString<int> (opts.GetArg());
            break;
        case 'l': {
            char c = opts.GetArg()[0];
            mode = c == 'b' ? LM_BLOCK : c == 'c' ? LM_CONTEXT : LM_NONE;
            break;
        }
        }
    }

    return !(configDir.empty() || filesDir.empty() || LM_NONE == mode);
}

}

int main(int argc, const char** argv) {
    using namespace NSegutils;
    TString configDir, filesDir;
    ELinksMode mode = LM_NONE;
    ui32 max = Max<ui32> ();

    if (!GetOpts(argc, argv, configDir, filesDir, mode, max))
        exit(0);

    TParserContext ctx(configDir);
    TFileList l;
    l.Fill(filesDir);
    THtmlFileReader r(THtmlFileReader::MDM_FirstLine);
    TStats newstats;

    ui32 n = 0;
    const char* fn = nullptr;
    while ((fn = l.Next()) && n < max) {
        THtmlFile data = r.Read(fn);
        TStat stat = ProcessDoc(data, ctx, mode);

        if (!stat.Total())
            continue;

        Clog << ++n << "\t" << data.FileName << "\t" << stat.Total() << "\t"
                << stat.Pr() << "\t" << stat.Re() << Endl;

        newstats.push_back(stat);
    }

    ProcessStats(newstats);

    return 0;
}
