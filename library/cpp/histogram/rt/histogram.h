#pragma once

#include <util/generic/map.h>

namespace NJson {
    class TJsonValue;
}

namespace NRTYServer {
    namespace NProto {
        class THistogram;
        class THistograms;
    }

    enum class EHistogramReportTraits: ui32 {
        Full = 1,
        FrqR = 1 << 1,
        CountR = 1 << 2,
        Frq = 1 << 3,
        Count = 1 << 4,
    };

    using THistogramReportTraits = ui32;
    static THistogramReportTraits HistogramFullReport = Max<THistogramReportTraits>();

    class THistogram {
    public:
        THistogram() {
        }
        THistogram(const TString& name, ui64 range)
            : Name(name)
            , Range(range)
        {
        }

        inline void Add(ui64 value) {
            Values[GetRangeStart(value)] += 1;
        }
        inline ui64 Get(ui64 value) const {
            const auto p = Values.find(GetRangeStart(value));
            return (p != Values.end()) ? p->second : 0;
        }
        inline const TString& GetName() const {
            return Name;
        }
        inline ui64 GetRange() const {
            return Range;
        }

        NJson::TJsonValue GetPublicReport(const THistogramReportTraits traits = HistogramFullReport, const ui64 border = Max<ui64>()) const;

        void Merge(const THistogram& other);

        void Serialize(NRTYServer::NProto::THistogram& proto) const;
        void Serialize(NJson::TJsonValue& json) const;
        void Deserialize(const NRTYServer::NProto::THistogram& proto);
        void Deserialize(const NJson::TJsonValue& json);

        template <class T>
        inline T Serialize() const {
            T container;
            Serialize(container);
            return container;
        }

        bool GetTrustBorder(const double pBorder, ui64& result) const;

    private:
        inline ui64 GetRangeStart(ui64 value) const {
            return value - (value % Range);
        }
        inline ui64 GetRangeEnd(ui64 value) const {
            return GetRangeStart(value) + Range - 1;
        }

    private:
        TString Name;
        ui64 Range = Max<ui64>();

        TMap<ui64, ui64> Values;
    };

    class THistograms {
    public:
        THistogram& AddAttribute(const TString& name, ui64 range);
        THistogram& GetAttribute(const TString& name);
        bool HasAttribute(const TString& name);

        void Merge(const THistograms& other);

        void Serialize(const TString& file) const;
        void Serialize(NRTYServer::NProto::THistograms& proto) const;
        void Serialize(NJson::TJsonValue& json) const;
        void Deserialize(const TString& file);
        void Deserialize(const NRTYServer::NProto::THistograms& proto);
        void Deserialize(const NJson::TJsonValue& json);

        template <class T>
        inline T Serialize() const {
            T container;
            Serialize(container);
            return container;
        }

    private:
        THistogram& AddAttribute(THolder<THistogram>&& histogram);

    private:
        TMap<TString, THolder<THistogram>> Histograms;
    };
}
