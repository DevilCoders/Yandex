#include <util/generic/array_ref.h>
#include <util/generic/cast.h>
#include <util/network/init.h>
#include <util/string/builder.h>
#include <util/system/env.h>
#include <util/system/tempfile.h>
#include <util/system/unaligned_mem.h>

#include <linux/ipv6.h>
#include <netinet/ip.h>

#include "p0f_format.h"

namespace {

    constexpr size_t MIN_TCP4 = sizeof(struct iphdr) + sizeof(struct tcphdr);
    constexpr size_t MIN_TCP6 = sizeof(struct ipv6hdr) + sizeof(struct tcphdr);

    enum class EBitFields {
        IpIdSet = 1,
        IpFlowSet = 1 << 1,
        TcpSeqZero = 1 << 2,
        TcpAckNotzeroNotset = 1 << 3,
        TcpAckZeroSet = 1 << 4,
        TcpUrgNotzeroNotset = 1 << 5,
        TcpUrgSet = 1 << 6,
        TcpPush = 1 << 7
    };

    enum class ETcpOption: ui8 {
        Eol = 0,
        Nop = 1,
        Mss = 2,
        Ws = 3,
        SackPermitted = 4,
        Sack = 5,
        Ts = 8
    };

    struct TTcpOptions {
        TMaybe<ui8> Eol;
        TMaybe<ui16> MaxSegmentSize;
        TMaybe<ui8> WindowScale;
        TMaybe<ui32> Timestamp1;
        TMaybe<ui32> Timestamp2;

        bool Malformed = false;
        bool TrailingNonZeroData = false;
        TVector<ui8> Layout;
    };

    NP0f::TP0fOrError CreateP0fValue(TString value) {
        return NP0f::TP0fOrError{std::move(value), Nothing()};
    }

    /// Figure out if window size is a multiplier of MSS or MTU. We don't take
    /// window scaling into account, because neither do TCP stack developers.
    /// Original version https://github.com/p0f/p0f/blob/master/fp_tcp.c#L50
    /// Function expects that only SYN packets are passed to
    static i16 DetectWinMulti(const p0f_value_t& p, const TTcpOptions& options,
                              bool& useMtu) {
        const ui16 win = p.wsize;
        const i32 mss = options.MaxSegmentSize.GetOrElse(0), mss12 = mss - 12;

        if (!win || mss < 100) {
            return -1;
        }

#define RET_IF_DIV(div, useMtu_)       \
    do {                               \
        if ((div) && !(win % (div))) { \
            useMtu = (useMtu_);        \
            return win / (div);        \
        }                              \
    } while (0)

        RET_IF_DIV(mss, false);
        RET_IF_DIV(mss12, false);

        /// Some systems use MTU on the wrong interface, so let's check for the most
        /// common case.
        RET_IF_DIV(1500 - MIN_TCP4, false);
        RET_IF_DIV(1500 - MIN_TCP4 - 12, false);

        if (p.version == 6) {
            RET_IF_DIV(1500 - MIN_TCP6, false);
            RET_IF_DIV(1500 - MIN_TCP6 - 12, false);
        }

        /// Some systems use MTU instead of MSS:
        RET_IF_DIV(mss + MIN_TCP4, true);
        RET_IF_DIV(mss + p.tot_hdr, true);
        if (p.version == 6) {
            RET_IF_DIV(mss + MIN_TCP6, true);
        }
        RET_IF_DIV(1500, true);

#undef RET_IF_DIV

        return -1;
    }

    constexpr ui8 MAX_DIST = 35;

    ui8 GuessDistance(ui8 ttl) {
        if (ttl <= 32) {
            return 32 - ttl;
        }
        if (ttl <= 64) {
            return 64 - ttl;
        }
        if (ttl <= 128) {
            return 128 - ttl;
        }
        return 255 - ttl;
    }

    TTcpOptions ParseOptions(const TArrayRef<const ui8>& options) {
        TTcpOptions result;
        bool stop = false;

        for (size_t i = 0; i < options.size() && !stop;) {
            const auto kind = options[i];
            switch (kind) {
                case ToUnderlying(ETcpOption::Eol):
                    result.Layout.push_back(kind);
                    result.Eol = options.size() - i - 1;
                    ++i;
                    while (i < options.size() && options[i] == 0) {
                        ++i;
                    }
                    if (i != options.size()) {
                        result.TrailingNonZeroData = true;
                        i = options.size();
                    }
                    break;
                case ToUnderlying(ETcpOption::Nop):
                    result.Layout.push_back(kind);
                    ++i;
                    break;
                case ToUnderlying(ETcpOption::Mss):
                    result.Layout.push_back(kind);
                    if (i + 4 > options.size()) {
                        result.Malformed = true;
                        stop = true;
                        break;
                    }
                    if (options[i + 1] != 4) {
                        result.Malformed = true;
                    }
                    result.MaxSegmentSize = ntohs(ReadUnaligned<ui16>(&options[i + 2]));
                    i += 4;
                    break;
                case ToUnderlying(ETcpOption::Ws):
                    result.Layout.push_back(kind);
                    if (i + 3 > options.size()) {
                        result.Malformed = true;
                        stop = true;
                        break;
                    }
                    if (options[i + 1] != 3) {
                        result.Malformed = true;
                    }
                    result.WindowScale = options[i + 2];
                    i += 3;
                    break;
                case ToUnderlying(ETcpOption::SackPermitted):
                    result.Layout.push_back(kind);
                    if (i + 2 > options.size()) {
                        result.Malformed = true;
                        stop = true;
                        break;
                    }
                    if (options[i + 1] != 2) {
                        result.Malformed = true;
                    }
                    i += 2;
                    break;
                case ToUnderlying(ETcpOption::Sack):
                    result.Layout.push_back(kind);
                    if (i + 2 > options.size() || i + options[i + 1] > options.size() ||
                        options[i + 1] < 10 || options[i + 1] > 34) {
                        result.Malformed = true;
                        stop = true;
                        break;
                    }
                    i += options[i + 1];
                    break;
                case ToUnderlying(ETcpOption::Ts):
                    result.Layout.push_back(kind);
                    if (i + 10 > options.size()) {
                        result.Malformed = true;
                        stop = true;
                        break;
                    }
                    if (options[i + 1] != 10) {
                        result.Malformed = true;
                    }
                    result.Timestamp1 = ntohl(ReadUnaligned<ui32>(&options[i + 2]));
                    result.Timestamp2 = ntohl(ReadUnaligned<ui32>(&options[i + 6]));
                    i += 10;
                    break;
                default:
                    if (i + 2 > options.size() || i + options[i + 1] > options.size() ||
                        options[i + 1] < 2) {
                        result.Malformed = true;
                        stop = true;
                        break;
                    }
                    i += options[i + 1];
            }
        }

        return result;
    }

    void SerializeLayout(const TTcpOptions& options, TStringBuilder& out) {
        bool first = true;
        for (const auto option : options.Layout) {
            if (!first) {
                out << ",";
            }
            switch (option) {
                case ToUnderlying(ETcpOption::Eol):
                    // If Eol has no value, it's error
                    out << "eol+" << static_cast<ui32>(options.Eol.GetOrElse(0));
                    break;
                case ToUnderlying(ETcpOption::Nop):
                    out << "nop";
                    break;
                case ToUnderlying(ETcpOption::Mss):
                    out << "mss";
                    break;
                case ToUnderlying(ETcpOption::Ws):
                    out << "ws";
                    break;
                case ToUnderlying(ETcpOption::SackPermitted):
                    out << "sok";
                    break;
                case ToUnderlying(ETcpOption::Sack):
                    out << "sack";
                    break;
                case ToUnderlying(ETcpOption::Ts):
                    out << "ts";
                    break;
                default:
                    out << "?" << static_cast<ui32>(option);
                    break;
            }
            first = false;
        }
    }

    void SerializeQuirks(const p0f_value_t& p, const TTcpOptions& options,
                         TStringBuilder& out) {
        bool first = true;
        auto printQuirk = [&](TStringBuf quirk) {
            if (!first) {
                out << ",";
            }
            out << quirk;
            first = false;
        };

        if (p.version == 4) {
            const bool df = p.flags & IP_DF;
            const bool ipIdSet = p.bit_fields & static_cast<int>(EBitFields::IpIdSet);
            if (df) {
                printQuirk("df");
            }
            if (df && ipIdSet) {
                printQuirk("id+");
            } else if (!df && !ipIdSet) {
                printQuirk("id-");
            }
        }
        if (p.ecn) {
            printQuirk("ecn");
        }
        if (p.version == 4 && (p.flags & IP_RF)) {
            printQuirk("0+");
        }
        if (p.version == 6 &&
            (p.bit_fields & static_cast<int>(EBitFields::IpFlowSet))) {
            printQuirk("flow");
        }

        if (p.bit_fields & static_cast<int>(EBitFields::TcpSeqZero)) {
            printQuirk("seq-");
        }
        if (p.bit_fields & static_cast<int>(EBitFields::TcpAckNotzeroNotset)) {
            printQuirk("ack+");
        }
        if (p.bit_fields & static_cast<int>(EBitFields::TcpAckZeroSet)) {
            printQuirk("ack-");
        }
        if (p.bit_fields & static_cast<int>(EBitFields::TcpUrgNotzeroNotset)) {
            printQuirk("uptr+");
        }
        if (p.bit_fields & static_cast<int>(EBitFields::TcpUrgSet)) {
            printQuirk("urgf+");
        }
        if (p.bit_fields & static_cast<int>(EBitFields::TcpPush)) {
            printQuirk("pushf+");
        }

        if (options.Timestamp1 == 0u) {
            printQuirk("ts1-");
        }
        if (options.Timestamp2 &&
            *options.Timestamp2 !=
                0) { /// In the original p0f code here is the check that the packet is
                     /// SYN but we only fingerprint SYN packets
            printQuirk("ts2+");
        }
        if (options.TrailingNonZeroData) {
            printQuirk("opt+");
        }
        if (options.WindowScale > 14) {
            printQuirk("exws");
        }
        if (options.Malformed) {
            printQuirk("bad");
        }
    }

} // namespace

NP0f::TP0fOrError NP0f::CreateP0fError(TString error) {
    return NP0f::TP0fOrError{Nothing(), std::move(error)};
}

NP0f::TP0fOrError NP0f::FormatP0f(const p0f_value_t& p) {
    if (p.opts_size > std::size(p.opts)) {
        NP0f::CreateP0fError("tcp options size is too large");
    }

    TArrayRef<const ui8> opts(reinterpret_cast<const ui8*>(p.opts), p.opts_size);
    const auto options = ParseOptions(opts);

    const auto mss = options.MaxSegmentSize.GetOrElse(0);
    const ui32 scale = options.WindowScale.GetOrElse(0);
    const auto dist = GuessDistance(p.ittl);
    bool useMtu = false;
    const auto winMulti = DetectWinMulti(p, options, useMtu);

    TStringBuilder result;
    result << static_cast<ui32>(p.version) << ":" << static_cast<ui32>(p.ittl)
           << "+";
    if (dist > MAX_DIST) {
        result << "?";
    } else {
        result << static_cast<ui32>(dist);
    }
    result << ":" << static_cast<ui32>(p.olen) << ":" << mss << ":";
    if (winMulti != -1) {
        result << (useMtu ? "mtu" : "mss") << "*" << winMulti;
    } else {
        result << p.wsize;
    }
    result << "," << scale << ":";
    SerializeLayout(options, result);
    result << ":";
    SerializeQuirks(p, options, result);
    result << ":" << (p.pclass == 0 ? "0" : "+");

    return CreateP0fValue(std::move(result));
}
