#include "pcapdump.h"

#include "unsorted.h"

#include <library/cpp/getopt/opt.h>

#include <util/memory/tempbuf.h>
#include <util/generic/yexception.h>
#include <util/generic/queue.h>
#include <util/digest/multi.h>
#include <util/draft/ip.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/string/printf.h>
#include <util/string/vector.h>
#include <util/system/guard.h>
#include <util/system/env.h>

#include <library/cpp/streams/factory/factory.h>
#include <library/cpp/http/fetch/httpparser.h>

#include <cctype>
#include <cstring>
#include <ctime>
#include <algorithm>

#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>

/* Fact: timestamp reordering in pcap files happens, e.g. due to offloading.
 *
 * Request timestamp may be a) ts of the first request data packet b) ts of
 * the last request data packet, c) ts of the first/last ACK. (a) is considered
 * true.
 *
 * Request may be stored as soon as the end-of-request is detected. There
 * are several ways to do that: a) Response is given b) Request is parsed
 * and validated. (b) is more robust, supports pipelining and uni-direction
 * capture, (a) requires Request to be stored in RAM till first byte of
 * Response comes and does not need any protocol parser, so this
 * implementation does (b).
 *
 * Response is skipped at the moment, it gives TTFB and TTLB metrics. Both
 * are measured using the data packet timestamp.
 */

static constexpr ui32 PCAP_MAGIC = 0xA1B2C3D4u;
static constexpr ui32 DLT_EN10MB = 1;

// Pcap headers are shamelessly stolen from https://wiki.wireshark.org/Development/LibpcapFileFormat
// Pcap file header
struct THdrPcapGlb {
    ui32 magic_number;   /* magic number */
    ui16 version_major;  /* major version number */
    ui16 version_minor;  /* minor version number */
    i32  thiszone;       /* GMT to local correction */
    ui32 sigfigs;        /* accuracy of timestamps */
    ui32 snaplen;        /* max length of captured packets, in octets */
    ui32 network;        /* data link type */

    bool operator==(const THdrPcapGlb& rhs) const {
        return memcmp(this, &rhs, sizeof(*this)) == 0;
    }
};

// Pcap frame header
struct THdrPcapRec {
    ui32 ts_sec;         /* timestamp seconds */
    ui32 ts_usec;        /* timestamp microseconds */
    ui32 incl_len;       /* number of octets of packet saved in file */
    ui32 orig_len;       /* actual length of packet */
};

using TMaybeFrameHeader = TMaybeFail<THdrPcapRec>;

// IP-addresses are in network byte order, ports are in host byte-order.
// IPv4 addresses are mapped to ::ffff:a.b.c.d
struct TFlowAddr {
    TIp6 SrcAddr;
    TIp6 DstAddr;
    TIpPort SrcPort;
    TIpPort DstPort;

    TFlowAddr Reverse() const {
        return {DstAddr, SrcAddr, DstPort, SrcPort};
    }

    bool operator==(const TFlowAddr& rhs) const {
        return SrcAddr == rhs.SrcAddr &&
               DstAddr == rhs.DstAddr &&
               SrcPort == rhs.SrcPort &&
               DstPort == rhs.DstPort;
    }

    std::pair<TIp6, std::pair<TIp6, std::pair<TIpPort, TIpPort>>> Tuple() const {
        return std::make_pair(SrcAddr, std::make_pair(DstAddr, std::make_pair(SrcPort, DstPort)));
    }

    bool operator<(const TFlowAddr& rhs) const {
        return Tuple() < rhs.Tuple();
    }
};

template <>
void Out<TFlowAddr>(IOutputStream& os, const TFlowAddr& a) {
    os << "[" << a.SrcAddr << "]:" << a.SrcPort << "->[" << a.DstAddr << "]:" << a.DstPort;
}

template <>
struct THash<TFlowAddr> {
    inline size_t operator()(const TFlowAddr& a) const {
        return MultiHash(a.SrcAddr, a.DstAddr, a.SrcPort, a.DstPort);
    }
};

struct TPcapFrame {
    TPcapFrame(
            const TInstant& ts,
            const TFlowAddr& addr,
            const ui32 seq,
            const TString& data
    ) : Ts(ts)
      , Addr(addr)
      , Seq(seq)
      , Data(data)
    { }

    TInstant Ts;
    TFlowAddr Addr;
    ui32 Seq; // seq is in host byte-order
    TString Data;
};

struct TPcapFrameNewer {
    bool operator()(const TPcapFrame& lhs, const TPcapFrame& rhs) const {
        return rhs.Ts < lhs.Ts; // NB: rhs/lhs swapped
    }
};

// pcap file may have a-bit-reordered timestamps, so TPcapFrameQueue sorts them
using TPcapFrameQueue = TPriorityQueue<TPcapFrame, TDeque<TPcapFrame>, TPcapFrameNewer>;

// In both the PAWS and the RTTM mechanism, the "timestamps" are 32-bit
// unsigned integers in a modular 32-bit space.  Thus, "less than" is defined
// the same way it is for TCP sequence numbers, and the same implementation
// techniques apply.  If s and t are timestamp values, s < t if 0 < (t - s) < 2**31,
// computed in unsigned 32-bit arithmetic.
// -- RFC 1323 -- TCP Extensions for High Performance
bool TcpSeqLess(const ui32 s, const ui32 t) {
    const ui32 diff = t - s;
    if (0 < diff && diff < 0x4000000u) {
        return true;
    } else if (0xC000000u < diff || diff == 0) {
        return false;
    }
    ythrow yexception()
        << "TCP SEQ overflow, implement PAWS. "
        "s=" << Sprintf("0x%08x", s) << ", "
        "t=" << Sprintf("0x%08x", t) << ", "
        "diff=" << Sprintf("0x%08x", diff) << Endl;
}

struct TTcpSeqNewer {
    bool operator()(const TPcapFrame& lhs, const TPcapFrame& rhs) const {
        return TcpSeqLess(rhs.Seq, lhs.Seq); // NB: rhs/lhs swapped!
    }
};

using TTcpFrameQueue = TPriorityQueue<TPcapFrame, TDeque<TPcapFrame>, TTcpSeqNewer>;

struct TRequestInfo {
    TFlowAddr Addr;
    TInstant ReqFirst; // it's first request packet, it's not first packet of parsed request!
    TInstant ReqLast;
    TString Blob;
};

struct TResponseInfo {
    TFlowAddr Addr;
    TInstant ReqFirst;
    TInstant ReqLast;
    TInstant RespFirst;
    TInstant RespLast;
    int HttpStatus;
    ui64 BodyLength;
};

using TMaybeSeq = TMaybeFail<ui32>;
using TMaybeRequest = TMaybeFail<TRequestInfo>;
using TMaybeResponse = TMaybeFail<TResponseInfo>;

bool IsHttpRespBeginning(const TStringBuf& s) {
    THttpHeader rh;
    THttpHeaderParser p;
    p.Init(&rh);
    int result = p.Execute(s.data(), s.size());
    return result == p.NeedMore || result == p.Accepted;
}

bool IsHttpReqBeginning(const TStringBuf& s) {
    THttpRequestHeader rh;
    THttpHeaderParser p;
    p.Init(&rh);
    int result = p.Execute(s.data(), s.size());
    return result == p.NeedMore || result == p.Accepted;
}

class TBodyCounter {
    ui64 BodyLength_;

public:
    TBodyCounter()
        : BodyLength_(0)
    {
    }

    ui64 BodyLength() const { return BodyLength_; }

    void InitBodyCounter() {
        BodyLength_ = 0;
    }

    void CheckDocPart(const void* /* buf */, size_t len, THttpHeader* /* header */) {
        BodyLength_ += len;
    }
};

using TCouningParser = THttpParser<TBodyCounter>;

// Single TCP flow. Adding single packet emits single TMaybeRequest or
// TMaybeResponse as following assumptions are taken:
// - full Response follows full Request (no pipelining)
// - next Request follows full Response (no overlapping)
class TFrameFlow {
public:
    enum EState {
        WaitReq, // absolutely idle flow
        WaitFirstReqPacket, // first packet was "lost", ReqNextSeq_ is bad, ReqFirst_ is good
        ReqHeadFirstPacketOnTop,
        ReqHeadOngoing, // feeding ReqParser_
        ReqBodyOngoing, // decremending ReqBodyToSkip_

        WaitResp,
        WaitFirstRespPacket,
        RespFirstPacketOnTop,
        RespOngoing,
    };

    TFrameFlow(const TFlowAddr& addr)
        : State_(WaitReq)
        , Addr_(addr)
        , ReqBodyToSkip_(Max<decltype(ReqBodyToSkip_)>())
    {
    }

    EState State() const {
        const bool a = Request_.size(); // packet flow from A
        const bool b = Response_.size(); // packet flow from B
        const bool req = ReqChunks_.size();
        const bool rbo = ReqBodyToSkip_ != Max<decltype(ReqBodyToSkip_)>();
        const bool t1 = ReqFirst_ != TInstant::Zero();
        const bool t2 = ReqLast_ != TInstant::Zero();
        const bool t3 = RespFirst_ != TInstant::Zero();
        const bool t4 = RespLast_ != TInstant::Zero();
        switch (State_) {
            case WaitReq:
                Y_ASSERT(!a && !b && !req && !rbo && !t1 && !t2 && !t3 && !t4); break;
            case WaitFirstReqPacket:
                Y_ASSERT( a && !b && !req && !rbo &&  t1 &&        !t3 && !t4); break;
            case ReqHeadFirstPacketOnTop:
                Y_ASSERT( a && !b && !req && !rbo &&  t1 &&        !t3 && !t4); break;
            case ReqHeadOngoing:
                Y_ASSERT(      !b &&                  t1 &&        !t3 && !t4); break;
            case ReqBodyOngoing:
                Y_ASSERT(      !b &&  req &&  rbo &&  t1 &&  t2 && !t3 && !t4); break;
            case WaitResp:
                Y_ASSERT(!a &&       !req && !rbo &&  t1 &&  t2 && !t3 && !t4); break;
            case WaitFirstRespPacket:
                Y_ASSERT(!a &&  b && !req && !rbo &&  t1 &&  t2 &&  t3       ); break;
            case RespFirstPacketOnTop:
                Y_ASSERT(!a &&  b && !req && !rbo &&  t1 &&  t2 &&  t3       ); break;
            case RespOngoing:
                Y_ASSERT(!a &&       !req && !rbo &&  t1 &&  t2 &&  t3       ); break;
            default:
                Y_FAIL("Invalid state in %s", ToString(*this).data());
        }
        return State_;
    }

    const TMaybeSeq& ReqSeq() const { return ReqNextSeq_; }
    const TMaybeSeq& RespSeq() const { return RespNextSeq_; }

    TMaybeRequest AddRequest(const TPcapFrame& f) {
        Y_ASSERT(f.Addr == Addr());

        switch (State()) {
            case WaitReq:
            case WaitFirstReqPacket:
            case ReqHeadOngoing:
            case ReqBodyOngoing:
                return HandleIncomingRequest(f);

            default:
                Y_FAIL("Invalid state in %s", ToString(*this).data());
        }
    }

    TMaybeResponse AddResponse(const TPcapFrame& f) {
        Y_ASSERT(f.Addr == Addr().Reverse());
        Response_.push(f);
        if (RespNextSeq_.Defined() && !SkipRetransmitted(&Response_, *RespNextSeq_))
            return {};
        for (;;) {
            switch (State()) {
                case WaitResp:
                    RespFirst_ = f.Ts;
                    SetState(WaitFirstRespPacket);
                    break;

                case WaitFirstRespPacket:
                    if (IsHttpRespBeginning(Response_.top().Data)) {
                        RespNextSeq_ = Response_.top().Seq;
                        SetState(RespFirstPacketOnTop);
                    } else if (IsHttpRespBeginning(f.Data)) {
                        static bool log;
                        SimpleCallOnce(log, [&]() {
                            Cerr << "It happens: bad response frame in " << *this << ": " << Response_.top().Data.Quote() << Endl;
                        });
                        do {
                            Response_.pop();
                            Y_ASSERT(Response_);
                        } while (!IsHttpRespBeginning(Response_.top().Data));
                        RespNextSeq_ = Response_.top().Seq;
                        SetState(RespFirstPacketOnTop);
                    } else {
                        // neither top packet nor this packet look like HTTP response
                        return {};
                    }
                    break;

                case RespFirstPacketOnTop:
                case RespOngoing:
                    return HandleIncomingResponse();

                default:
                    Y_FAIL("Invalid state in %s", ToString(*this).data());
            }
        }
    }

    const TFlowAddr& Addr() const {
        return Addr_;
    }

    TInstant Timestamp() const {
        Y_ASSERT(State() != WaitReq);
        return ReqFirst_;
    }

private:
    void SetState(EState next) {
        State_ = next;
        State(); // run Y_ASSERT
    }

    TMaybeRequest HandleIncomingRequest(const TPcapFrame& f) {
        auto s = State();
        Request_.push(f);
        if (ReqNextSeq_.Defined() && !SkipRetransmitted(&Request_, *ReqNextSeq_))
            return {};
        for (;;) {
            switch (s) {
                case WaitReq:
                    ReqFirst_ = f.Ts; // timestamps are ordered in TPcapFrameQueue
                    SetState(WaitFirstReqPacket);
                    break;

                case WaitFirstReqPacket:
                    if (IsHttpReqBeginning(Request_.top().Data)) {
                        ReqNextSeq_ = Request_.top().Seq;
                        SetState(ReqHeadFirstPacketOnTop);
                    } else if (IsHttpReqBeginning(f.Data)) {
                        static bool log;
                        SimpleCallOnce(log, [&]() {
                            Cerr << "It happens: drop bad heading frame in " << *this << Endl;
                        });
                        do {
                            Request_.pop();
                            Y_ASSERT(Request_);
                        } while (!IsHttpReqBeginning(Request_.top().Data));
                        ReqNextSeq_ = Request_.top().Seq;
                        SetState(ReqHeadFirstPacketOnTop);
                    } else {
                        // neither top packet nor this packet look like HTTP header
                        return {};
                    }
                    break;

                case ReqHeadFirstPacketOnTop:
                case ReqHeadOngoing:
                case ReqBodyOngoing:
                    return HandleIncomingRequest();

                default:
                    Y_FAIL("Invalid state in %s", ToString(*this).data());
            }
            s = State();
        }
    }

    bool SkipRetransmitted(TTcpFrameQueue* queue, const ui32 next_seq) {
        while (!queue->empty() && TcpSeqLess(queue->top().Seq, next_seq)) {
            static bool log;
            SimpleCallOnce(log, [&]() {
                Cerr << "It happens: drop retransmission in " << *this << Endl;
            });
            queue->pop();
        }
        return !queue->empty();
    }

    bool SkipFrameQueueToCurrent(TTcpFrameQueue* queue, const ui32 next_seq) {
        return SkipRetransmitted(queue, next_seq) && queue->top().Seq == next_seq;
    }

    class TopRequestFrameGuard: public TNonCopyable {
        TFrameFlow& Flow_;

    public:
        TopRequestFrameGuard(TFrameFlow& flow)
            : Flow_(flow)
        {
            Y_ASSERT(Flow_.Request_ && Flow_.Request_.top().Seq == *Flow_.ReqNextSeq_);
        }

        ~TopRequestFrameGuard() {
            Y_ASSERT(Flow_.Request_ && Flow_.Request_.top().Seq == *Flow_.ReqNextSeq_);
            const TPcapFrame& f = Flow_.Request_.top();
            Flow_.ReqNextSeq_ = *Flow_.ReqNextSeq_ + static_cast<ui32>(f.Data.size());
            Flow_.ReqLast_ = std::max(Flow_.ReqLast_, f.Ts);
            Flow_.ReqChunks_.push_back(f.Data);
            Flow_.Request_.pop();
        }
    };

    TMaybeRequest HandleIncomingRequest() {
        for (;;) {
            switch (State()) {
                case ReqHeadFirstPacketOnTop:
                    ReqParser_.Init(&ReqHeader_);
                    SetState(ReqHeadOngoing);
                    break;

                case ReqHeadOngoing: {
                    if (!SkipFrameQueueToCurrent(&Request_, *ReqNextSeq_))
                        return {}; // retransmissions are not accounted to `ReqLast_`
                    TMaybeFail<TopRequestFrameGuard> guard;
                    guard.ConstructInPlace(*this);
                    const TPcapFrame& f = Request_.top();
                    const auto p = ReqParse(f.Data);
                    if (p.code == ReqParser_.NeedMore) {
                        static bool log;
                        SimpleCallOnce(log, [&]() {
                            Cerr << "It happens: request spans several frames in " << *this << Endl;
                        });
                    } else if (p.code == ReqParser_.Accepted) {
                        Y_ENSURE(ReqHeader_.transfer_chunked == -1,
                                 "Unsupported chunked request in " << *this);
                        const ui64 body_len = std::max(ReqHeader_.content_length, 0l);
                        const size_t pkt_body_len = f.Data.size() - p.taken;
                        const size_t pkt_body_taken = std::min(pkt_body_len, body_len);
                        Y_ENSURE(f.Data.size() == p.taken + pkt_body_taken, "Unsupported pipelining in " << *this);
                        ReqBodyToSkip_ = body_len - pkt_body_taken;
                        guard.Clear(); // `f` is invalid afterwards
                        if (ReqBodyToSkip_) {
                            SetState(ReqBodyOngoing);
                        } else {
                            return EmitRequestAndWaitResp();
                        }
                    } else {
                        // XXX: guard APPENDS data to `ReqChunks_` anyway
                        ythrow yexception() << "Request parser code " << p.code << " in " << *this;
                    }
                }; break;

                case ReqBodyOngoing: {
                    Y_ASSERT(ReqBodyToSkip_);
                    if (!SkipFrameQueueToCurrent(&Request_, *ReqNextSeq_))
                        return {}; // retransmissions are not accounted to `ReqLast_`
                    TMaybeFail<TopRequestFrameGuard> guard;
                    guard.ConstructInPlace(*this);
                    const TPcapFrame& f = Request_.top();
                    const size_t pkt_body_len = f.Data.size();
                    const size_t pkt_body_taken = std::min(pkt_body_len, ReqBodyToSkip_);
                    Y_ENSURE(f.Data.size() == pkt_body_taken, "Unsupported pipelining in " << *this);
                    ReqBodyToSkip_ -= pkt_body_taken;
                    if (!ReqBodyToSkip_) {
                        guard.Clear(); // `f` is invalid afterwards
                        return EmitRequestAndWaitResp();
                    }
                }; break;

                default:
                    Y_FAIL("Invalid state in %s", ToString(*this).data());
            }
        }
    }

    TMaybeRequest EmitRequestAndWaitResp() {
        Y_ASSERT(ReqFirst_ != TInstant::Zero() && ReqLast_ != TInstant::Zero() && ReqChunks_ && ReqFirst_ <= ReqLast_ && ReqBodyToSkip_ != Max<decltype(ReqBodyToSkip_)>());
        TMaybeRequest ret(TRequestInfo{Addr(), ReqFirst_, ReqLast_, JoinStrings(ReqChunks_, "")});
        ReqChunks_.clear();
        ReqBodyToSkip_ = Max<decltype(ReqBodyToSkip_)>();
        SetState(WaitResp);
        return ret;
    }

    TMaybeResponse EmitResponseAndWaitReq() {
        Y_ASSERT(ReqFirst_ <= ReqLast_ && ReqLast_ < RespFirst_ && RespFirst_ <= RespLast_);
        TMaybeResponse ret(TResponseInfo{
            Addr(),
            ReqFirst_,
            ReqLast_,
            RespFirst_,
            RespLast_,
            RespHeader_.http_status,
            RespParser_.BodyLength()
        });
        SkipRetransmitted(&Request_, *ReqNextSeq_);
        SkipRetransmitted(&Response_, *RespNextSeq_);
        ReqFirst_ = ReqLast_ = RespFirst_ = RespLast_ = TInstant::Zero();
        SetState(WaitReq);
        return ret;
    }

    class TopResponseFrameGuard: public TNonCopyable {
        TFrameFlow& Flow_;
    public:
        TopResponseFrameGuard(TFrameFlow& flow)
            : Flow_(flow)
        {
            Y_ASSERT(Flow_.Response_ && Flow_.Response_.top().Seq == *Flow_.RespNextSeq_);
        }

        ~TopResponseFrameGuard() {
            Y_ASSERT(Flow_.Response_ && Flow_.Response_.top().Seq == *Flow_.RespNextSeq_);
            const TPcapFrame& f = Flow_.Response_.top();
            Flow_.RespNextSeq_ = *Flow_.RespNextSeq_ + static_cast<ui32>(f.Data.size());
            Flow_.RespLast_ = std::max(Flow_.RespLast_, f.Ts);
            Flow_.Response_.pop();
        }
    };

    TMaybeResponse HandleIncomingResponse() {
        for (;;) {
            switch (State()) {
                case RespFirstPacketOnTop:
                    RespParser_.Init(&RespHeader_);
                    RespParser_.InitBodyCounter();
                    SetState(RespOngoing);
                    break;

                case RespOngoing: {
                    if (!SkipFrameQueueToCurrent(&Response_, *RespNextSeq_))
                        return {}; // retransmissions are not accounted to `ReqLast_`
                    TMaybeFail<TopResponseFrameGuard> guard;
                    guard.ConstructInPlace(*this);
                    const auto before = RespParser_.GetState();
                    Y_ASSERT(before != RespParser_.hp_eof && before != RespParser_.hp_error);
                    // FIXME: pipelining is silently broken -- library/cpp/http/fetch/httpparser_ut.cpp
                    const TPcapFrame& f = Response_.top();
                    RespParser_.Parse((void*)f.Data.data(), f.Data.size());
                    const auto after = RespParser_.GetState();
                    if (after == RespParser_.hp_eof) {
                        guard.Clear();

                        return EmitResponseAndWaitReq();
                    } else if (after == RespParser_.hp_error) {
                        ythrow yexception() << "Response parser code "
                            << Sprintf("%d (%s) in ", RespHeader_.error, ExtHttpCodeStr(RespHeader_.error).data())
                            << *this;
                    }
                }; break;

                default:
                    Y_FAIL("Invalid state in %s", ToString(*this).data());
            }
        }
    }

    struct TReqParserOut {
        int code;
        size_t taken;
    };

    TReqParserOut ReqParse(const TStringBuf& s) {
        TReqParserOut o;
        o.taken = Max<size_t>();
        o.code = ReqParser_.Execute(s.data(), s.size());
        if (o.code == ReqParser_.Accepted) {
            o.taken = (ReqParser_.lastchar - s.data()) + 1;
            Y_ASSERT(o.taken <= s.size());
        }
        return o;
    }

    EState State_;
    const TFlowAddr Addr_;

    TTcpFrameQueue Request_;
    TTcpFrameQueue Response_;

    TMaybeSeq ReqNextSeq_;
    TMaybeSeq RespNextSeq_;
    ui64 ReqBodyToSkip_;

    THttpRequestHeader ReqHeader_;
    THttpHeaderParser ReqParser_;
    TVector<TString> ReqChunks_; // temporary storage for full request

    TCouningParser RespParser_;
    THttpHeader RespHeader_;

    TInstant ReqFirst_;
    TInstant ReqLast_;
    TInstant RespFirst_;
    TInstant RespLast_;
};


#define ENUM_OUT(x) case TFrameFlow:: x : os << #x; break;
template <>
void Out<TFrameFlow::EState>(IOutputStream& os, TFrameFlow::EState state) {
    switch (state) {
        ENUM_OUT(WaitReq);
        ENUM_OUT(WaitFirstReqPacket);
        ENUM_OUT(ReqHeadFirstPacketOnTop);
        ENUM_OUT(ReqHeadOngoing);
        ENUM_OUT(ReqBodyOngoing);
        ENUM_OUT(WaitResp);
        ENUM_OUT(WaitFirstRespPacket);
        ENUM_OUT(RespFirstPacketOnTop);
        ENUM_OUT(RespOngoing);
    }
}
#undef ENUM_OUT

template <>
void Out<TFrameFlow>(IOutputStream& os, const TFrameFlow& f) {
    os << "TFrameFlow[" << f.Addr() << " " << f.State();
    if (f.State() != f.WaitReq) {
        os << " " << f.Timestamp();
    }
    os << "]";
}

using TFrameFlowPtr = TSimpleSharedPtr<TFrameFlow>;

template <class T>
struct TPreferOldest {
    bool operator()(const T& lhs, const T& rhs) const {
        return lhs.ReqFirst > rhs.ReqFirst;
    }
};

using TReqKey = std::pair<TInstant, TFlowAddr>;

template <class T>
TReqKey MakeKey(const T& r) {
    return {r.ReqFirst, r.Addr};
}

template <>
TReqKey MakeKey(const TFrameFlow& flow) {
    return {flow.Timestamp(), flow.Addr()};
}

using TIncompleteFlowHash = THashMap<TFlowAddr, TFrameFlowPtr>;
using TIncompleteFlowTimeline = TMap<TReqKey, TFrameFlowPtr>;
using TCompleteReqQueue = TPriorityQueue<TRequestInfo, TDeque<TRequestInfo>, TPreferOldest<TRequestInfo>>;
using TCompleteRespQueue = TPriorityQueue<TResponseInfo, TDeque<TResponseInfo>, TPreferOldest<TResponseInfo>>;

/* Pcap File Input Srteam */
class TPcapInput {
public:
    class ISink {
    public:
        virtual ~ISink() { }
        virtual void WriteRequest(const TRequestInfo&) { }
        virtual void WriteResponse(const TResponseInfo&) { }
    };

    inline TPcapInput(ui16 srv_port, IInputStream* slave, ISink* sink)
        : PcapReorderingWindow_(PcapReorderingWindow())
        , ServerPort_(srv_port)
        , Slave_(slave)
        , Sink_(sink)
        , PktNo_(0)
        , FirstfileHeader_(LoadPcapHeader(slave))
        , FileNo_(1)
    {
    }

    void Flush() {
        FlushFrameQueue();
        FlushDoneReqQueue();
        FlushDoneRespQueue();
    }

    bool ReadPacket() {
        PktNo_++;
        try {
            return TryReadTcpPacket();
        } catch (yexception e) {
            Cerr << e << ", packet #" << PktNo_ << Endl;
            throw;
        }
    }

private:
    static size_t PcapReorderingWindow() {
        // Single buffered packet is enough for my captures. `16` is just a safety net.
        // I assume this value will never be tuned.
        const TString p = GetEnv("D_PCAP_REORDERING_WINDOW");
        return p ? FromString<size_t>(p) : 16;
    }

    static THdrPcapGlb LoadPcapHeader(IInputStream* slave) {
        THdrPcapGlb fhdr;
        slave->LoadOrFail(&fhdr, sizeof(THdrPcapGlb));
        Y_ENSURE(fhdr.magic_number != htonl(PCAP_MAGIC), "Unsupported big-endian pcap file");
        Y_ENSURE(fhdr.magic_number == PCAP_MAGIC, "Not a pcap file, bad magic " << Sprintf("0x%08x", fhdr.magic_number));
        // `thiszone` does not matter as the code does not use ABSOLUTE timestamps
        Y_ENSURE(fhdr.network == DLT_EN10MB, "Unsupported non-ethernet data link type " << fhdr.network);
        return fhdr;
    }

    TMaybeFrameHeader TryReadFrameHeader() {
        union {
            THdrPcapGlb File;
            THdrPcapRec Frame;
        } hdr;
        constexpr size_t headSize = sizeof(hdr.Frame);
        constexpr size_t tailSize = sizeof(hdr) - sizeof(hdr.Frame);
        const size_t realLen = Slave_->Load(&hdr, headSize);
        if (Y_UNLIKELY(realLen == 0)) {
            return {};
        } else if (Y_UNLIKELY(realLen != headSize)) { // copy-paste from LoadOrFail
            ythrow yexception() << "Failed to read required number of bytes from stream! Expected: " << headSize << ", gained: " << realLen << "!";
        }
        if (memcmp(&hdr, &FirstfileHeader_, headSize) == 0) { // same magic/version/thiszone/sigfigs
            Slave_->LoadOrFail(reinterpret_cast<ui8*>(&hdr) + headSize, tailSize);
            Y_ENSURE(hdr.File == FirstfileHeader_, "pcap concatenated detected, but file #" << FileNo_ << " and #" << FileNo_ + 1 << " have different format"); // snaplen, network
            FileNo_++;
            Slave_->LoadOrFail(&hdr, headSize);
        }
        return hdr.Frame;
    }

    bool TryReadTcpPacket() {
        TMaybeFrameHeader maybeHeader = TryReadFrameHeader();
        if (!maybeHeader.Defined())
            return false;
        const THdrPcapRec& hdr = maybeHeader.GetRef();
        Y_ENSURE(hdr.incl_len <= hdr.orig_len && hdr.orig_len <= 0x20000,
                "Invalid IP packet size, wire=" << hdr.orig_len << ", pcap=" << hdr.incl_len);
        Y_ENSURE(hdr.incl_len == hdr.orig_len, "Truncated frames are not supported");
        TString buf = TString::Uninitialized(hdr.incl_len);
        Slave_->LoadOrFail(buf.begin(), hdr.incl_len);
        TStringBuf tail(buf);

        Y_ENSURE(sizeof(ether_header) <= tail.size(), "Incomplete ethernet frame");
        const ether_header* eth = reinterpret_cast<const ether_header*>(tail.begin()); // SIGBUS is buried here
        tail.Skip(sizeof(*eth));

        // Skip IP header
        TFlowAddr addr;
        const tcphdr* tcp = nullptr;
        if (eth->ether_type == htons(ETHERTYPE_IP)) {
            Y_ENSURE(sizeof(iphdr) <= tail.size(), "Incomplete IPv4 frame");
            const iphdr* ipv4 = reinterpret_cast<const iphdr*>(tail.begin());
            Y_ENSURE(ipv4->version == 4 && ipv4->ihl >= 5, "Damaged IPv4 frame");
            if (ipv4->protocol != IPPROTO_TCP)
                return true;
            const ui16 tot_len = ntohs(ipv4->tot_len);
            Y_ENSURE(tot_len <= tail.size(), "Truncated IPv4 frame, tot_len=" << tot_len << ", tail.Size()=" << tail.size());
            tail.Trunc(tot_len);
            Y_ENSURE(!(ntohs(ipv4->frag_off) & 0x1fff), "Unsupported fragmented IPv4 packet");
            tail.Skip(ipv4->ihl * 4);
            Y_ENSURE(sizeof(tcphdr) <= tail.size(), "Incomplete IPv4/TCP frame");
            tcp = reinterpret_cast<const tcphdr*>(tail.begin());

            addr.SrcAddr = Ip6FromIp4(ipv4->saddr);
            addr.DstAddr = Ip6FromIp4(ipv4->daddr);
        } else if (eth->ether_type == htons(ETHERTYPE_IPV6)) {
            Y_ENSURE(sizeof(ip6_hdr) <= tail.size(), "Incomplete IPv6 frame");
            const ip6_hdr* ipv6 = reinterpret_cast<const ip6_hdr*>(tail.begin());
            const ui16 version = (ipv6->ip6_vfc >> 4);
            Y_ENSURE(version == 6, "Damaged IPv6 frame, version=" << version);
            if (ipv6->ip6_nxt != IPPROTO_TCP)
                return true;
            tail.Skip(sizeof(ip6_hdr));
            const ui16 pad_len = ntohs(ipv6->ip6_plen);
            Y_ENSURE(pad_len <= tail.size(), "Truncated IPv6 frame, ip6_plen=" << pad_len << ", tail.Size()=" << tail.size());
            tail.Trunc(pad_len);
            tcp = reinterpret_cast<const tcphdr*>(tail.begin());
            memcpy(&addr.SrcAddr.Data, &ipv6->ip6_src, sizeof(addr.SrcAddr.Data));
            memcpy(&addr.DstAddr.Data, &ipv6->ip6_dst, sizeof(addr.DstAddr.Data));
        } else {
            return true;
        }

        Y_ENSURE(tcp->doff >= 5, "Damaged TCP frame");
        Y_ENSURE(tcp->doff * 4 <= tail.size(), "Truncated TCP frame, doff=" << tcp->doff << ", tail.Size()=" << tail.size());
        Y_ENSURE(!tcp->urg, "Unsupported URG flag");
        tail.Skip(tcp->doff * 4);

        addr.SrcPort = ntohs(tcp->source);
        addr.DstPort = ntohs(tcp->dest);

        Y_ENSURE(addr.SrcPort == ServerPort_ || addr.DstPort == ServerPort_, "Unexpected TCP stream, it goes not to " << Sprintf("ServerPort=%u, source=%u, dest=%u", ServerPort_, addr.SrcPort, addr.DstPort));

        if (!tail)
            return true; // Drop bare `ACK`

        Y_ENSURE(!tcp->syn, "Unsupported TFO, got SYN with data");

        FrameQueue_.emplace(
            TInstant::MicroSeconds(static_cast<ui64>(hdr.ts_sec) * 1000000 + hdr.ts_usec),
            addr,
            ntohl(tcp->seq),
            ToString(tail)
        );
        ProcessFrameQueue();

        return true;
    }

    void ProcessFrameQueue() {
        while (FrameQueue_.size() > PcapReorderingWindow_)
            TakeFrameFromQueue();
    }

    void FlushFrameQueue() {
        while (FrameQueue_.size())
            TakeFrameFromQueue();
    }

    void TakeFrameFromQueue() {
        const TPcapFrame& f = FrameQueue_.top();
        Y_ENSURE(LastFrameTs_ <= f.Ts, "Major timestamp reordering in `pcap` file near " << f.Ts
                 << " and " << LastFrameTs_ << ", try to increase D_PCAP_REORDERING_WINDOW envvar above " << PcapReorderingWindow_);
        LastFrameTs_ = f.Ts;
        if (f.Addr.DstPort == ServerPort_) { // request data
            ProcessRequestFrame(f);
        } else { // response data
            Y_ASSERT(f.Addr.SrcPort == ServerPort_);
            ProcessResponseFrame(f);
        }
        FrameQueue_.pop();
    }

    static bool IsRetransmission(const TMaybeSeq& seq, const TPcapFrame& frame) {
        // FIXME: not robust, dies on RANDOM injected sequence in TcpSeqLess PAWS-check
        return seq.Defined() && TcpSeqLess(frame.Seq, *seq);
    }

    void ProcessRequestFrame(const TPcapFrame& f) {
        const TFlowAddr& key = f.Addr;
        TFrameFlowPtr flow;
        auto todo = TodoReqHash_.find(key);
        if (todo == TodoReqHash_.end()) { // new request
            auto badresp = TodoRespHash_.find(key);
            if (badresp != TodoRespHash_.end()) {
                if (IsRetransmission(badresp->second->ReqSeq(), f))
                    return;
                DropFlowCryCerr(badresp->second);
            }
            flow = MakeSimpleShared<TFrameFlow>(key);
        } else {
            flow = todo->second;
            if (IsRetransmission(flow->ReqSeq(), f))
                return;
            const auto s = flow->State();
            Y_ASSERT(s == flow->WaitReq || // existing idle stream
                     s == flow->WaitFirstReqPacket ||
                     s == flow->ReqHeadOngoing ||
                     s == flow->ReqBodyOngoing);
        }

        TMaybeRequest req;
        try {
            req = flow->AddRequest(f);
        } catch (const yexception& e) {
            Cerr << e << " in " << *flow << Endl;
            DropFlowCryCerr(flow);
            flow.Reset();
        }

        if (flow.Get()) {
            if (req.Defined()) {
                Y_ASSERT(flow->State() == flow->WaitResp);
                TodoReqHash_.erase(flow->Addr());
                TodoReqTimeline_.erase(MakeKey(*flow));
                TodoRespHash_[flow->Addr()] = flow;
            } else if (flow->State() != flow->WaitReq) {
                const auto s = flow->State();
                Y_ASSERT(s == flow->WaitFirstReqPacket ||
                         s == flow->ReqHeadOngoing ||
                         s == flow->ReqBodyOngoing);
                TodoReqHash_[flow->Addr()] = flow;
                TodoReqTimeline_[MakeKey(*flow)] = flow;
                Y_ASSERT(!TodoRespHash_.contains(flow->Addr()));
            }
        }

        if (req.Defined()) {
            DoneReq_.push(req.GetRef());
            ProcessDoneReqQueue();
        }
    }

    void ProcessResponseFrame(const TPcapFrame& f) {
        const TFlowAddr& key = f.Addr.Reverse();
        TFrameFlowPtr flow;
        auto todo = TodoRespHash_.find(key);
        if (todo != TodoRespHash_.end()) { // new request
            flow = todo->second;
            if (IsRetransmission(flow->RespSeq(), f))
                return;
            const auto s = flow->State();
            Y_ASSERT(s == flow->WaitResp ||
                     s == flow->WaitFirstRespPacket ||
                     s == flow->RespOngoing);
        } else {
            static bool log;
            SimpleCallOnce(log, [&]() {
                Cerr << "It happens: retransmission or response without corresponding request in " << key << " at " << f.Ts << Endl;
            });
            return;
        }

        TMaybeResponse resp;
        try {
            resp = flow->AddResponse(f);
        } catch (const yexception& e) {
            Cerr << e << " in " << *flow << Endl;
            DropFlowCryCerr(flow);
            flow.Reset();
        }

        if (flow.Get()) {
            Y_ASSERT(!TodoReqHash_.contains(flow->Addr()));
            if (resp.Defined()) {
                Y_ASSERT(flow->State() == flow->WaitReq);
                TodoRespHash_.erase(flow->Addr());
                TodoReqHash_[flow->Addr()] = flow; // to preserve seq/ack continuity
            } else {
                const auto s = flow->State();
                Y_ASSERT(s == flow->WaitFirstRespPacket ||
                         s == flow->RespOngoing);
                TodoRespHash_[flow->Addr()] = flow;
            }
        }

        if (resp.Defined()) {
            DoneResp_.push(resp.GetRef());
            ProcessDoneRespQueue();
        }
    }

    void DropFlowCryCerr(TFrameFlowPtr flow) {
        TVector<TString> containers;
        if (TodoReqHash_.erase(flow->Addr()) > 0) {
            containers.push_back("incomplete-request");
        }
        if (flow->State() != flow->WaitReq) {
            if (TodoReqTimeline_.erase(MakeKey(*flow)) > 0) {
                containers.push_back("incomplete-request-timeline");
            }
        }
        if (TodoRespHash_.erase(flow->Addr()) > 0) {
            containers.push_back("incomplete-response");
        }
        if (containers) {
            Cerr << "Dropping " << *flow << " from [" << JoinStrings(containers, ", ") << "]" << Endl;
        }
    }

    TInstant OldestIncomplete() const {
        return TodoReqTimeline_ ? TodoReqTimeline_.begin()->first.first : TInstant::Max();
    }

    void ProcessDoneReqQueue() {
        // Can process Done request that is older than ANY incomplete one as
        // requests must be properly ordered in plan file.
        while (DoneReq_ && DoneReq_.top().ReqFirst < OldestIncomplete())
            TakeDoneReq();
    }

    void FlushDoneReqQueue() {
        while (DoneReq_)
            TakeDoneReq();
    }

    void TakeDoneReq() {
        const TRequestInfo& req = DoneReq_.top();
        Sink_->WriteRequest(req);
        LastWrittenRequest_ = req.ReqFirst;
        DoneReq_.pop();
        ProcessDoneRespQueue();
    }

    void ProcessDoneRespQueue() {
        // Can process Done response only after corresponding request as `ruid`
        // has to be extracted. Using `LastWrittenRequest_` as a barrier is not
        // the very best option, but it eases handling of missing Responses.
        while (DoneResp_ && DoneResp_.top().ReqFirst < LastWrittenRequest_)
            TakeDoneResp();
    }

    void FlushDoneRespQueue() {
        while (DoneResp_)
            TakeDoneResp();
    }

    void TakeDoneResp() {
        const TResponseInfo& resp = DoneResp_.top();
        Sink_->WriteResponse(resp);
        DoneResp_.pop();
    }

private:
    const size_t PcapReorderingWindow_;
    const ui16 ServerPort_;

    IInputStream* Slave_;
    ISink* Sink_;
    ui64 PktNo_;
    const THdrPcapGlb FirstfileHeader_;
    unsigned int FileNo_;

    TPcapFrameQueue FrameQueue_;
    TInstant LastFrameTs_;

    TIncompleteFlowHash TodoReqHash_;
    TIncompleteFlowTimeline TodoReqTimeline_;
    TCompleteReqQueue DoneReq_;
    TInstant LastWrittenRequest_;

    TIncompleteFlowHash TodoRespHash_;
    TCompleteRespQueue DoneResp_;
};

TPcapDumpLoader::TPcapDumpLoader()
    : Host_("localhost")
    , Port_(80)
    , ServerPort_(0)
{
}

TPcapDumpLoader::~TPcapDumpLoader() {
}

TString TPcapDumpLoader::Opts() {
    return "h:p:s:T:ar";
}

bool TPcapDumpLoader::HandleOpt(const TOption* option) {
    switch (option->key) {
        case 'h':
        case 'p':
        case 's':
        case 'T':
            if (!option->opt) {
                ythrow yexception() << "-p option requires argument";
            }
            break;

        case 'a':
        case 'r':
            ythrow yexception() << "-" << option->key << " was never documented and is gone now";

        default:
            return false;
    }

    switch (option->key) {
        case 'h':
            Host_ = option->opt;
            break;
        case 'p':
            try {
                Port_ = FromString<ui16>(option->opt);
            } catch (yexception e) {
                ythrow e << "also, `-p` is no longer `prefix for each request`, hostname should be set with `-h` option now";
            }
            break;
        case 's':
            ServerPort_ = FromString<ui16>(option->opt);
            break;
        case 'T':
            TimingsLog_ = option->opt;
            break;
    }

    return true;
}

void TPcapDumpLoader::Usage() const {
    // Cerr << "   -p prefix for each request (" << Prefix_ << ")" << Endl;
    Cerr << "   -h host to connect to: " << Host_ << Endl
         << "   -p port to connect to: " << Port_ << Endl
         << "   -s server port to filter TCP streams" << Endl
         << "   -T put known request/response timings in file.tsv" << Endl
    ;
}

template <typename T>
T AfterOrNone(T buf, typename T::char_type c) {
    T l, r;
    return buf.TrySplit(c, l, r) ? r : T{};
}

class TPcapPlanner: public TPcapInput::ISink, public TNonCopyable {
public:
    TPcapPlanner() = delete;
    TPcapPlanner(const TString& host, ui16 port, IReqsLoader::IOutputter* out, IOutputStream* out_times)
        : Out_(out)
        , OutTimes_(out_times)
        , Host_(host)
        , Port_(port)
        , LastReqTs_(TInstant::Zero())
    {
        Y_ASSERT(out);
        if (OutTimes_) {
            *OutTimes_ << "ruid\treq_first\treq_last\tresp_first\tresp_last\treq_length\thttp_status\tbody_length\n";
        }
    }

    void Flush() {
        for (const auto& pair : C_) {
            const auto& val = pair.second;
            Cerr << "Lost response for ruid=" << val.Ruid.Quote() << ", req_first=" << val.ReqFirst << ", req_last=" << val.ReqLast << Endl;
        }
        C_.clear();
    }

    virtual void WriteRequest(const TRequestInfo& req) override {
        Y_ASSERT(LastRespTs_ <= LastReqTs_ && LastReqTs_ <= req.ReqFirst);
        if (LastReqTs_ == TInstant::Zero()) {
            LastReqTs_ = req.ReqFirst;
        }
        if (OutTimes_) {
            const auto it = C_.find(req.Addr);
            if (it != C_.end()) {
                const auto& val = it->second;
                Cerr << "Lost response for ruid=" << val.Ruid.Quote() << ", req_first=" << val.ReqFirst << ", req_last=" << val.ReqLast << Endl;
            }
            C_[req.Addr] = {req.ReqFirst, req.ReqLast, req.Blob.size(), TString(ExtractRuid(req.Blob))};
        }
        Out_->Add(TDevastateItem{
            req.ReqFirst - LastReqTs_,
            Host_,
            Port_,
            req.Blob,
            0
        });
        // Cout << C_[MakeKey(req)].Ruid.Quote() << "\t" << req.ReqFirst.MicroSeconds() << "\t" << req.ReqLast.MicroSeconds() << "\t" << req.Blob.Quote() << Endl;
        LastReqTs_ = req.ReqFirst;
    }

    virtual void WriteResponse(const TResponseInfo& resp) override {
        Y_ASSERT(LastRespTs_ <= LastReqTs_); // responses may be written out-of-order
        LastRespTs_ = Max(resp.ReqFirst, LastRespTs_);
        if (OutTimes_) {
            auto it = C_.find(resp.Addr);
            Y_ENSURE(it->second.ReqFirst == resp.ReqFirst && it->second.ReqLast == resp.ReqLast, "BUG: Request / Responses mismatch in " << resp.Addr);
            Y_ENSURE(it != C_.end(), "BUG: Response handled before corresponding Request in " << resp.Addr);
            *OutTimes_
                << it->second.Ruid << "\t"
                << resp.ReqFirst.MicroSeconds() << "\t"
                << resp.ReqLast.MicroSeconds() << "\t"
                << resp.RespFirst.MicroSeconds() << "\t"
                << resp.RespLast.MicroSeconds() << "\t"
                << it->second.BlobLength << "\t"
                << resp.HttpStatus << "\t"
                << resp.BodyLength << "\n";
            C_.erase(it);
        }
    }

private:
    struct TReqCache {
        TInstant ReqFirst;
        TInstant ReqLast;
        ui64 BlobLength;
        TString Ruid;
    };

    TStringBuf ExtractRuid(TStringBuf blob) {
        const char* ruid = "ruid=";
        blob = blob.Before('\r');
        blob = AfterOrNone(blob, ' ');
        blob = blob.Before(' ');
        blob = AfterOrNone(blob, '?');
        while (blob && !blob.StartsWith(ruid)) {
            blob = AfterOrNone(blob, '&');
        }
        if (blob.StartsWith(ruid)) {
            blob = blob.Skip(strlen(ruid));
            blob = blob.Before('&');
        } else {
            Y_ASSERT(!blob);
        }
        return blob;
    }

private:
    IReqsLoader::IOutputter* Out_;
    IOutputStream* OutTimes_;
    const TString Host_;
    const ui16 Port_;
    TInstant LastReqTs_;
    TInstant LastRespTs_;
    THashMap<TFlowAddr, TReqCache> C_;
};

void TPcapDumpLoader::Process(TParams* params) {
    TAutoPtr<IOutputStream> timings;
    if (TimingsLog_)
        timings = OpenOutput(TimingsLog_);
    TPcapPlanner out(Host_, Port_, params->Outputter(), timings.Get());
    TPcapInput pi(ServerPort_, params->Input(), &out);
    while (pi.ReadPacket()) {
        ;
    }
    pi.Flush();
    out.Flush();
    if (timings)
        timings->Flush();
}
