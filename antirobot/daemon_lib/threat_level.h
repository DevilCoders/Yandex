#pragma once

#include "captcha_freq_checker.h"
#include "factors.h"
#include "req_types.h"
#include "rps_calc.h"

#include <util/ysaveload.h>
#include <util/datetime/base.h>

namespace NAntiRobot {
    struct TRequestFeatures;

    struct TRequestProcessInfo {
        TRequestProcessInfo(TRawFactors* reqFactors,
                            TInstant arrivalTime,
                            bool isAccountableRequest,
                            bool isImportantRequest)
            : ReqFactors(reqFactors)
            , ArrivalTime(arrivalTime)
            , IsAccountableRequest(isAccountableRequest)
            , IsImportantRequest(isImportantRequest)
        {
        }

        TRawFactors* ReqFactors;
        TInstant ArrivalTime;
        bool IsAccountableRequest;
        bool IsImportantRequest;
    };

    struct TRpsStat {
        float Rps;
        EBlockCategory Category;

        TRpsStat()
            : Rps(0)
            , Category(BC_UNDEFINED)
        {
        }

        TRpsStat(float smoothRps, EBlockCategory category)
            : Rps(smoothRps)
            , Category(category)
        {
        }
    };

    class TThreatLevel {
    public:
        typedef TVector<char> TCompressedData;

        TThreatLevel();
        ~TThreatLevel();

        void ProcessRequest(const TRequestFeatures& rf, TFactorsWithAggregation& factorValues);
        void ProcessReqFactors(const TRequestProcessInfo& rpi, TFactorsWithAggregation& factorValues);
        void ProcessCaptchaInput(ui64 arrivalTime, bool success);

        void Touch(TInstant now) noexcept {
            TimeStamp = Max(TimeStamp, now);
        }

        void TouchWithAggregation(TInstant now);

        TInstant LastTimeStamp() const {
            return TimeStamp;
        }

        void SetFlushed(TInstant now) noexcept {
            LastFlushTime = now;
        }

        TInstant GetLastFlushTime() const {
            return LastFlushTime;
        }

        ui32 GetNumImportantReqs() const {
            return NumImportantReqs;
        }

        void UpdateNumImportantReqs(ui32 numImportantReqs) {
            if (numImportantReqs > NumImportantReqs)
                NumImportantReqs = numImportantReqs;
        }

        size_t GetCompressedDataSize() const {
            return CompressedData.capacity();
        }

        void Clear();
        void Load(IInputStream* in);
        void Save(IOutputStream* out) const;

        static size_t GetUncompressedDataSize();

        bool UpdateCaptchaInputFreq(ui64 arrivalTime, bool& wasBanned) {
            return CaptchaFreqCheck.CheckForBan(arrivalTime, wasBanned);
        }

        void CurRpsStat(EBlockCategory category, TRpsStat& rpsStat) const;

        inline void TouchDDosRps(EBlockCategory type, const TInstant& arrivalTime) {
            TouchRps(int(type), arrivalTime);
        }

        inline void ClearRps(int ddosType) {
            FreqData.RequestFrequencies[ddosType].Clear();
        }

    private:
        struct TFreqData {
            TDiscreteRpsCounter RequestFrequencies[BC_NUM];

            TFreqData();
            void Clear();
        };

        inline void TouchRps(int ddosType, const TInstant& arrivalTime) {
            FreqData.RequestFrequencies[ddosType].ProcessRequest(arrivalTime);
        }

    private:
        ui32 FactorsVersion;
        TInstant TimeStamp;
        TInstant LastFlushTime;
        ui32 NumImportantReqs;
        ui32 NumReqs;
        float AggregatedDelta;
        TCaptchaInputFrequencyChecker CaptchaFreqCheck;
        TFreqData FreqData;
        TCompressedData CompressedData;
    };
}

Y_DECLARE_PODTYPE(NAntiRobot::TThreatLevel::TFreqData);
