#include "replier.h"

#include <kernel/common_server/library/unistat/signals.h>
#include <kernel/common_server/library/openssl/aes.h>
#include <library/cpp/streams/lz/lz.h>
#include <util/stream/zlib.h>
#include <util/stream/buffer.h>
#include <kernel/reqid/reqid.h>

namespace NRTProcHistogramSignals {
    const TVector<double> TimeIntervals = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200, 300, 400, 500, 1000, 2000 };
}

namespace {
    NCS::NObfuscator::TObfuscatorKeyMap MakeObfuscatorKey(const TString& contentType, const TString& uri)  {
        auto key = NCS::NObfuscator::TObfuscatorKeyMap(contentType);
        key.Add("Uri", uri);
        return key;
    }

    NCS::NObfuscator::TObfuscatorKeyMap MakeHeaderObfuscatorKey(const TString& contentType, const TString& header)  {
        auto key = NCS::NObfuscator::TObfuscatorKeyMap(contentType);
        key.Add("header", header);
        return key;
    }
}

TAtomic IReplyContext::GlobalRequestsCounter = 0;

void IReplyContext::Init(NCS::NLogging::TBaseLogRecord& record, const NCS::NObfuscator::TObfuscatorManagerContainer& obfuscatorManager) {
    const TBaseServerRequestData& rd = GetBaseRequestData();
    record.Add("ip", NUtil::GetClientIp(rd));
    record.Add("raw_ip", rd.RemoteAddr());
    record.Add("query", rd.Query());
    record.Add("reqstamp", rd.RequestBeginTime());
    record.Add("uri", TString(GetUri()) + "?" + GetBaseRequestData().Query());

    auto reqid = NUtil::GetReqId(GetBaseRequestData(), GetCgiParameters());
    if (reqid) {
        record.Add("reqid", reqid);
    } else {
        auto generatedReqId = ReqIdGenerate();
        record.Add("reqid", generatedReqId);
    }

    ci_equal_to equals;
    for (auto&& i : rd.HeadersIn()) {
        const TString& key = i.first;
        TString value = i.second;

        if (equals(key, TStringBuf("Content-Type"))) {
            const TBlob& blob = GetBuf();
            if (!blob.Empty()) {
                TStringBuf post(blob.AsCharPtr(), blob.Size());
                if (value.find("image/") != TStringBuf::npos) {
                    record.Add("body", Base64Encode(post), 16 * 1024);
                } else {
                    record.Add("body",  obfuscatorManager.Obfuscate(MakeObfuscatorKey(value, TString(GetUri())), post));
                }
                record.Add("request_body_length", post.Size());
            }
        }
        record.Add(key, obfuscatorManager.Obfuscate(MakeHeaderObfuscatorKey("header", key), value));
    }
}

EDeadlineCorrectionResult IReplyContext::DeadlineCorrection(const TDuration& scatterTimeout, const double kffWaitingAvailable) {
    TCgiParameters& cgi = MutableCgiParameters();
    TCgiParameters::iterator iter = cgi.find(TStringBuf("timeout"));
    if (iter == cgi.end() && scatterTimeout.MicroSeconds()) {
        cgi.InsertUnescaped("timeout", ToString(scatterTimeout.MicroSeconds()));
        iter = cgi.find(TStringBuf("timeout"));
    }

    if (iter != cgi.end()) {
        ui64 timeout;
        if (!TryFromString(iter->second, timeout)) {
            return EDeadlineCorrectionResult::dcrIncorrectDeadline;
        }
        RequestDeadline = TInstant::MicroSeconds(GetRequestStartTime().MicroSeconds() + timeout);
    }
    return DeadlineCheck(kffWaitingAvailable);
}

EDeadlineCorrectionResult IReplyContext::DeadlineCheck(const double kffWaitingAvailable) const {
    if (RequestDeadline != TInstant::Max()) {
        if (Now() - GetRequestStartTime() >= (RequestDeadline - GetRequestStartTime()) * kffWaitingAvailable) {
            return EDeadlineCorrectionResult::dcrRequestExpired;
        }
        return EDeadlineCorrectionResult::dcrOK;
    } else {
        return EDeadlineCorrectionResult::dcrNoDeadline;
    }
}

TBuffer TReportEncryptor::Process(const TBuffer& report, IReportBuilderContext& context) const {
    TString encrypted;
    if (!NOpenssl::AESEncrypt(Key, report, encrypted)) {
        context.AddReplyInfo("SecretVersion", "0");
        return report;
    }
    context.AddReplyInfo("SecretVersion", ::ToString(SecretVersion));
    return TBuffer(encrypted.data(), encrypted.size());
}

TBuffer TReportCompressor::Process(const TBuffer& report, IReportBuilderContext& context) const {
    TBuffer compressed;
    TBufferOutput out(compressed);
    switch (Type) {
    case TReportCompressor::EType::Lz4:
        TLz4Compress(&out).Write(report.Data(), report.Size());
        break;
    case TReportCompressor::EType::GZip:
        TZLibCompress(&out, ZLib::GZip).Write(report.Data(), report.Size());
        break;
    default:
        break;
    }
    context.AddReplyInfo("Use-Custom-Compression", "true");
    return compressed;
}
