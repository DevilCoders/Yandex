#pragma once

#include "filter.h"
#include "printer.h"

#include <contrib/libs/libpcap/pcap.h>

#include <util/generic/hash.h>
#include <util/stream/format.h>
#include <util/string/cast.h>

#include <utility>

#include <arpa/inet.h>

namespace {
    TStringBuf GetPayload(const unsigned char* pkt) {
        if (pkt[0x0c] != 0x86 || pkt[0x0d] != 0xdd)
            return TStringBuf{}; // ethertype is not ipv6
        if ((pkt[0x0e] >> 4) != 6)
            return TStringBuf{}; // ip version is not 6
        if (pkt[0x14] != 6)
            return TStringBuf{}; // not tcp

        ui32 tcpPacketLen = 4u * (pkt[0x42] >> 4);
        size_t len = ntohs(*(ui16*)(pkt + 0x12)) - tcpPacketLen;
        return {(const char*)pkt + 0x36 + tcpPacketLen, len};
    }

    bool IsHttpResponse(TStringBuf buf) {
        return buf.StartsWith("HTTP/1.1 200 Ok") ||
               buf.StartsWith("HTTP/1.1 302 Moved temporarily") ||
               buf.StartsWith("HTTP/1.1 400 Bad request") ||
               buf.StartsWith("HTTP/1.1 403 Forbidden");
    }

    typedef TString TIpAddress;
    typedef TString TPort;

    std::pair<TString, TString> GetStreamId(const unsigned char* pkt) {
        TIpAddress sourceAddr((const char*)(pkt + 0x16), 0x10);
        TPort sourcePort((const char*)(pkt + 0x36), 2);
        return {sourceAddr, sourcePort};
    }

    std::pair<TString, TString> GetResponseStreamId(const unsigned char* pkt) {
        TIpAddress sourceAddr((const char*)(pkt + 0x26), 0x10);
        TPort sourcePort((const char*)(pkt + 0x38), 2);
        return {sourceAddr, sourcePort};
    }
}

class TAmmoGenerator {
public:
    virtual ~TAmmoGenerator() = default;

    virtual void Process(struct pcap_pkthdr header, const unsigned char* pkt) = 0;
    virtual void PrintStats() = 0;
};

class TCacherAmmoGenerator: public TAmmoGenerator {
public:
    TCacherAmmoGenerator(TAmmoPrinter* printer, TMaybe<TAmmoFilter> filter)
        : Printer(printer)
        , Filter(std::move(filter))
    {
    }

    void Process(struct pcap_pkthdr header, const unsigned char* pkt) override {
        if (header.len < 0x43)
            return; // too small for eth+ip+tcp+payload

        const bool hasResponseFilter = Filter.Defined();

        TStringBuf buf = GetPayload(pkt);
        if (buf.empty()) {
            return;
        }

        if (IsHttpResponse(buf)) {
            if (!hasResponseFilter) {
                return;
            }

            auto streamId = GetResponseStreamId(pkt);

            auto it = Streams.find(streamId);
            if (it == Streams.end()) {
                return;
            }

            const auto& requestChunks = it->second;

            if (Filter.GetRef()(buf, requestChunks)) {
                Printer->Print(requestChunks);
            }

            Streams.erase(it);
            return;
        }

        auto streamId = GetStreamId(pkt);

        auto request = buf;
        if (request.StartsWith("POST") || request.StartsWith("GET")) {
            if (IsFirstChunk(request)) {
                if (IsLastChunk(request) && !hasResponseFilter) {
                    Printer->Print(request);
                } else {
                    if (Streams.contains(streamId)) {
                        AbandonedStreams++;
                    }
                    Streams[streamId] = {ToString(request)};
                }
            } else {
                if (!hasResponseFilter) {
                    Printer->Print(request);
                } else {
                    if (Streams.contains(streamId)) {
                        AbandonedStreams++;
                    }
                    Streams[streamId] = {ToString(request)};
                }
            }
        } else {
            if (Streams.contains(streamId)) {
                if (IsLastChunk(request) && !hasResponseFilter) {
                    Streams[streamId].push_back(ToString(request));
                    Printer->Print(Streams[streamId]);
                    Streams.erase(streamId);
                } else {
                    Streams[streamId].push_back(ToString(request));
                }
            } else {
                if (IsHtmlPageResponse(buf)) {
                    return;
                }
                RequestsWithoutStart++;
            }
        }
    }
    void PrintStats() override {
        size_t unfinishedStreams = Streams.size();

        Cerr << "abandonedStreams = " << AbandonedStreams << Endl;
        Cerr << "requestsWithoutStart = " << RequestsWithoutStart << Endl;
        Cerr << "unfinishedStreams = " << unfinishedStreams << Endl;
    }

private:
    bool IsFirstChunk(TStringBuf request) {
        return request.StartsWith("POST /fullreq HTTP/1.1\r\nTransfer-Encoding: chunked");
    }

    bool IsLastChunk(TStringBuf request) {
        return request.EndsWith("0\r\n\r\n");
    }

    bool IsHtmlPageResponse(TStringBuf buf) {
        // Contains one of antirobot metrika urls
        return buf.Contains("//mc.yandex.ru/watch/10630330?") ||
               buf.Contains("//mc.yandex.ru/watch/10632040?") ||
               buf.Contains("//mc.yandex.ru/watch/17335522\"") ||
               buf.Contains("//mc.yandex.ru/watch/16275268\"");
    }

private:
    TAmmoPrinter* Printer;
    TMaybe<TAmmoFilter> Filter;
    THashMap<std::pair<TIpAddress, TPort>, TVector<TString>> Streams;
    size_t AbandonedStreams = 0;
    size_t RequestsWithoutStart = 0;
};

class TProcessorAmmoGenerator: public TAmmoGenerator {
public:
    explicit TProcessorAmmoGenerator(TAmmoPrinter* printer)
        : Printer(printer)
    {
    }
    void Process(struct pcap_pkthdr header, const unsigned char* pkt) override {
        if (header.len < 0x43)
            return; // too small for eth+ip+tcp+payload

        TStringBuf buf = GetPayload(pkt);
        if (buf.empty() || IsHttpResponse(buf)) {
            return;
        }

        std::pair<TString, TString> streamId = GetStreamId(pkt);
        auto request = buf;
        if (IsFirstChunk(request)) {
            if (Streams.contains(streamId)) {
                AbandonedStreams++;
            }
            Streams[streamId] = {ToString(request)};
        } else {
            if (!Streams.contains(streamId) || GetContentLength(Streams[streamId][0]) != request.size()) {
                RequestsWithoutStart++;
                return;
            }
            Streams[streamId].push_back(ToString(request));
            Printer->Print(Streams[streamId]);
            Streams.erase(streamId);
        }
    }

    void PrintStats() override {
        size_t unfinishedStreams = Streams.size();

        Cerr << "abandonedStreams = " << AbandonedStreams << Endl;
        Cerr << "requestsWithoutStart = " << RequestsWithoutStart << Endl;
        Cerr << "unfinishedStreams = " << unfinishedStreams << Endl;
    }

private:
    bool IsFirstChunk(TStringBuf request) {
        return request.StartsWith("POST /process HTTP/1.1");
    }

    size_t GetContentLength(const TString& s) {
        if (!s.StartsWith("POST /process")) {
            return 0;
        }
        static const TString headerName = "Content-Length:";
        size_t idx = 0;
        while (idx < s.size()) {
            while (idx + 1 < s.size() && s[idx] != 'C') {
                idx++;
            }
            bool ok = true;
            for (size_t i = 0; i < headerName.size(); i++) {
                if (s[idx + i] != headerName[i]) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                size_t res = 0;
                idx += headerName.size() + 1;
                while ('0' <= s[idx] && s[idx] <= '9') {
                    res = res * 10 + (size_t)(s[idx] - '0');
                    idx++;
                }
                return res;
            }
        }
        return 0;
    }

private:
    TAmmoPrinter* Printer;
    THashMap<std::pair<TIpAddress, TPort>, TVector<TString>> Streams;
    size_t AbandonedStreams = 0;
    size_t RequestsWithoutStart = 0;
};

THolder<TAmmoGenerator> CreateAmmoGenerator(bool processorAmmo, TAmmoPrinter* printer, TMaybe<TAmmoFilter> ammoFilter) {
    return processorAmmo ? THolder<TAmmoGenerator>(new TProcessorAmmoGenerator(printer))
                         : THolder<TAmmoGenerator>(new TCacherAmmoGenerator(printer, ammoFilter));
}
