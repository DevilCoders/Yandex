#include "threat_level.h"

#include "convert_factors.h"
#include "eventlog_err.h"
#include "request_features.h"
#include "timedelta_checker.h"

#include <library/cpp/streams/lz/lz.h>

#include <util/stream/mem.h>

using namespace NAntiRobot;

namespace {
    const float DELTA_AGGR_WEIGHT = 1.0f - pow(0.5f, 1.0f / 50.0f);
    const size_t MAGIC_VALUE = 0xDEADBEEFBAADF00D;

    struct TThreatLevelData {
        TTimer<ui64> ReqTimer;
        TTimeDeltaChecker TimeDeltaChecker;
        TTimeDeltaExpDeviation TimeDeltaDeviation;
        TTimeDelta CaptchaInputDelta;
        TTimeDelta IncorrectCaptchaInputDelta;

        TFactorsWithAggregation* FactorValues;

        TThreatLevelData(TFactorsWithAggregation* factorValues)
            : FactorValues(factorValues)
        {
        }

        void Save(IOutputStream* out) const {
            ::Save(out, ReqTimer);
            ::Save(out, TimeDeltaChecker);
            ::Save(out, TimeDeltaDeviation);
            ::Save(out, MAGIC_VALUE);
            ::Save(out, CaptchaInputDelta);
            ::Save(out, IncorrectCaptchaInputDelta);

            ::Save(out, TFactorsWithAggregation::RawFactorsWithAggregationCount());
            FactorValues->Save(out);
        }

        void Load(IInputStream* in) {
            ::Load(in, ReqTimer);
            ::Load(in, TimeDeltaChecker);
            ::Load(in, TimeDeltaDeviation);

            size_t removed;
            ::Load(in, removed);

            ::Load(in, CaptchaInputDelta);
            ::Load(in, IncorrectCaptchaInputDelta);

            size_t count = 0;
            ::Load(in, count);
            FactorValues->Load(in, count);
        }

        void LoadFromVersion(IInputStream* in, ui32 version) {
            ::Load(in, ReqTimer);
            ::Load(in, TimeDeltaChecker);
            ::Load(in, TimeDeltaDeviation);

            size_t removed;
            ::Load(in, removed);

            ::Load(in, CaptchaInputDelta);
            ::Load(in, IncorrectCaptchaInputDelta);

            size_t count = 0;
            ::Load(in, count);
            LoadAndConvertFactors(version, in, count, *FactorValues);
        }

        static void LoadAndConvertFactors(ui32 fromVersion, IInputStream* in, size_t count, TFactorsWithAggregation& factors) {
            TTempBuf buf(count * sizeof(float));
            try {
                ::LoadArray(in, reinterpret_cast<float*>(buf.Data()), count);
            } catch(...) {
                ythrow yexception() << "buf error in load: " << CurrentExceptionMessage();
            }

            ConvertFactors(fromVersion, (float*) buf.Data(), factors);
        }

        void ProcessReqFactors(const TRequestProcessInfo& rpi)
        {
            TRawFactors& f = FactorValues->NonAggregatedFactors;
            f = *rpi.ReqFactors;

            f.Factors[F_TIME_DELTA] = ReqTimer.Next(rpi.ArrivalTime.MicroSeconds());
            f.Factors[F_CAPTCHA_INPUT_DELTA] = CaptchaInputDelta.GetLastDelta();
            f.Factors[F_CAPTCHA_INCORRECT_INPUT_DELTA] = IncorrectCaptchaInputDelta.GetLastDelta();

            if (rpi.IsAccountableRequest) {
                f.Factors[F_DELTA_DEVIATION_FROM_EXP_DISTRIB] = TimeDeltaDeviation.ProcessRequest(rpi.ArrivalTime);
            }

            {
                TimeDeltaChecker.ProcessRequest(rpi.ArrivalTime, rpi.IsAccountableRequest);

                f.SetBool(F_BAD_TIME_DELTAS, TimeDeltaChecker.IsCritical());
            }

            FactorValues->Aggregate();
        }

        void ProcessCaptchaInput(ui64 time, bool inputSuccess) {
            if (inputSuccess)
                CaptchaInputDelta.Next(time);
            else
                IncorrectCaptchaInputDelta.Next(time);
        }
    };

    void Uncompress(TThreatLevel::TCompressedData& cmprData, TThreatLevelData& tlData) {
        if (!cmprData.empty()) {
            TFactorsWithAggregation* factorValues = tlData.FactorValues;
            try {
                TMemoryInput inp(&cmprData.front(), cmprData.size());
                TLzoDecompress decompr(&inp);
                ::Load(&decompr, tlData);
            } catch(...) {
                EVLOG_MSG << "decompression error: " << CurrentExceptionMessage();
                cmprData.resize(0);
                tlData = TThreatLevelData(factorValues);
            }
        }
    }

    void Compress(TThreatLevel::TCompressedData& cmprData, const TThreatLevelData& tlData) {
        try {
            TTempBufOutput out;
            try {
                TLzoCompress compr(&out);
                ::Save(&compr, tlData);
                compr.Finish();
            } catch(...) {
                ythrow yexception() << "buf error in compress: " << CurrentExceptionMessage();
            }

            cmprData.reserve(out.Filled() * 105 / 100);
            cmprData.resize(out.Filled());
            if (out.Filled() != 0)
                memcpy(&cmprData.front(), out.Data(), out.Filled());
        } catch(...) {
            EVLOG_MSG << "compression error: " << CurrentExceptionMessage();
            cmprData.resize(0);
        }
    }
}

TThreatLevel::TThreatLevel()
{
    Clear();
}

TThreatLevel::~TThreatLevel() {
}

TThreatLevel::TFreqData::TFreqData() {
    for (size_t i = 0; i < BC_NUM; i++)
        RequestFrequencies[i] = TDiscreteRpsCounter(ANTIROBOT_DAEMON_CONFIG.DDosSmoothFactor);
}

void TThreatLevel::TFreqData::Clear() {
    for (size_t i = 0; i < BC_NUM; i++)
        RequestFrequencies[i].Clear();
}

void TThreatLevel::Clear() {
    FactorsVersion = FACTORS_VERSION;
    TimeStamp = TInstant();
    LastFlushTime = TInstant();
    NumImportantReqs = 0;
    NumReqs = 0;
    AggregatedDelta = 0.0f;
    CompressedData.clear();
    FreqData.Clear();
}

void TThreatLevel::ProcessRequest(const TRequestFeatures& rf, TFactorsWithAggregation& factorValues)
{
    TRawFactors reqFactors;
    reqFactors.FillFromFeatures(rf);
    TRequestProcessInfo rpi = {
        &reqFactors,
        rf.Request->ArrivalTime,
        rf.Request->IsAccountableRequest(),
        rf.Request->IsImportantRequest(),
    };

    ProcessReqFactors(rpi, factorValues);
}

void TThreatLevel::ProcessReqFactors(const TRequestProcessInfo& rpi, TFactorsWithAggregation& factorValues) {
    TThreatLevelData tlData(&factorValues);

    Uncompress(CompressedData, tlData);

    Touch(rpi.ArrivalTime);

    if (rpi.IsImportantRequest)
        ++NumImportantReqs;

    tlData.ProcessReqFactors(rpi);

    Compress(CompressedData, tlData);
}

void TThreatLevel::ProcessCaptchaInput(ui64 arrivalTime, bool inputSuccess) {
    TFactorsWithAggregation factorValues;

    TThreatLevelData tlData(&factorValues);

    Uncompress(CompressedData, tlData);
    tlData.ProcessCaptchaInput(arrivalTime, inputSuccess);
    Compress(CompressedData, tlData);
}

void TThreatLevel::TouchWithAggregation(TInstant now) {
    TInstant realNow = Max(TimeStamp, now);
    float delta = TimeStamp.MicroSeconds() ? realNow.MicroSeconds() - TimeStamp.MicroSeconds() : 0.0f;
    AggregatedDelta = (1.f - DELTA_AGGR_WEIGHT) * AggregatedDelta + DELTA_AGGR_WEIGHT * delta;
    TimeStamp = realNow;
    ++NumReqs;
}

void TThreatLevel::Load(IInputStream* in) {
    ::Load(in, FactorsVersion);
    ::Load(in, TimeStamp);
    ::Load(in, LastFlushTime);
    ::Load(in, NumImportantReqs);
    ::Load(in, NumReqs);
    ::Load(in, AggregatedDelta);
    ::Load(in, CaptchaFreqCheck);
    ::Load(in, FreqData);

    if (FactorsVersion == FACTORS_VERSION)
        ::Load(in, CompressedData);
    else {
        TFactorsWithAggregation fv;
        TThreatLevelData tlData(&fv);

        TCompressedData tmpCmpr;
        ::Load(in, tmpCmpr);

        try {
            TMemoryInput inp(&tmpCmpr.front(), tmpCmpr.size());
            TLzoDecompress decompr(&inp);

            tlData.LoadFromVersion(&decompr, FactorsVersion);
        } catch(...) {
            fv = TFactorsWithAggregation();

            tlData = TThreatLevelData(&fv);
        }

        Compress(CompressedData, tlData);
        FactorsVersion = FACTORS_VERSION;
    }
}

void TThreatLevel::Save(IOutputStream* out) const {
    ::Save(out, FactorsVersion);
    ::Save(out, TimeStamp);
    ::Save(out, LastFlushTime);
    ::Save(out, NumImportantReqs);
    ::Save(out, NumReqs);
    ::Save(out, AggregatedDelta);
    ::Save(out, CaptchaFreqCheck);
    ::Save(out, FreqData);
    ::Save(out, CompressedData);
}

size_t TThreatLevel::GetUncompressedDataSize() {
    return sizeof(TThreatLevelData) - sizeof(TFactorsWithAggregation*) + sizeof(TFactorsWithAggregation);
}

void TThreatLevel::CurRpsStat(EBlockCategory category, TRpsStat&  rpsStat) const {
    const TDiscreteRpsCounter& freq = FreqData.RequestFrequencies[int(category)];

    rpsStat = TRpsStat(freq.GetRps(), category);
}
