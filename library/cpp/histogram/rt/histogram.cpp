#include "histogram.h"

#include <library/cpp/histogram/rt/histogram.pb.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/protobuf/util/pb_io.h>


bool NRTYServer::THistogram::GetTrustBorder(const double pBorder, ui64& border) const {
    if (Values.empty()) {
        return false;
    }
    if (pBorder <= 0) {
        border = Values.begin()->first;
        return true;
    }
    if (pBorder >= 1) {
        border = Values.rbegin()->first;
        return true;
    }
    ui64 sum = 0;
    for (auto&& i : Values) {
        sum += i.second;
    }

    if (!sum) {
        return false;
    }

    ui64 sumCurrent = 0;
    for (auto&& i : Values) {
        if (1.0 * (sum - sumCurrent) / sum <= 1 - pBorder) {
            border = i.first;
            return true;
        }
        sumCurrent += i.second;
    }
    if (1.0 * (sum - sumCurrent) / sum <= 1 - pBorder) {
        border = Values.rbegin()->first;
    }
    return true;
}

NJson::TJsonValue NRTYServer::THistogram::GetPublicReport(const THistogramReportTraits traits, const ui64 border) const {
    NJson::TJsonValue report(NJson::JSON_MAP);
    ui64 sum = 0;
    for (auto&& i : Values) {
        sum += i.second;
    }

    NJson::TJsonValue& values = report.InsertValue("full", NJson::JSON_ARRAY);
    NJson::TJsonValue& pr = report.InsertValue("pr", NJson::JSON_ARRAY);
    NJson::TJsonValue& cr = report.InsertValue("cr", NJson::JSON_ARRAY);
    NJson::TJsonValue& frq = report.InsertValue("p", NJson::JSON_ARRAY);
    NJson::TJsonValue& count = report.InsertValue("c", NJson::JSON_ARRAY);
    ui64 sumCurrent = 0;
    for (auto&& i : Values) {
        NJson::TJsonValue value;
        value["start"] = i.first;
        value["end"] = GetRangeEnd(i.first);
        value["count"] = i.second;
        value["count_l"] = sumCurrent;
        value["count_r"] = sum - sumCurrent;
        value["frq"] = 1.0 * i.second / sum;
        value["frq_r"] = 1.0 * (sum - sumCurrent) / sum;
        value["frq_l"] = 1.0 * (sumCurrent) / sum;
        if (traits & (ui32)EHistogramReportTraits::Full) {
            values.AppendValue(value);
        }
        if (traits & (ui32)EHistogramReportTraits::FrqR) {
            pr.AppendValue(1.0 * (sum - sumCurrent) / sum);
        }
        if (traits & (ui32)EHistogramReportTraits::CountR) {
            cr.AppendValue(sum - sumCurrent);
        }
        if (traits & (ui32)EHistogramReportTraits::Count) {
            count.AppendValue(i.second);
        }
        if (traits & (ui32)EHistogramReportTraits::Frq) {
            frq.AppendValue(1.0 * i.second / sum);
        }
        sumCurrent += i.second;
        if (i.first > border) {
            break;
        }
    }
    return report;
}

void NRTYServer::THistogram::Merge(const THistogram& other) {
    if (GetName() != other.GetName()) {
        throw yexception() << "cannot merge histograms with different names: " << GetName() << " != " << other.GetName();
    }
    if (GetRange() != other.GetRange()) {
        throw yexception() << "cannot merge histograms with different ranges: " << GetRange() << " != " << other.GetRange();
    }
    for (auto&& e : other.Values) {
        Values[e.first] += e.second;
    }
}

void NRTYServer::THistogram::Serialize(NRTYServer::NProto::THistogram& proto) const {
    proto.SetName(GetName());
    proto.SetRange(GetRange());

    for (auto p = Values.begin(); p != Values.end(); ++p) {
        auto element = proto.AddElement();
        element->SetStart(p->first);
        element->SetEnd(GetRangeEnd(p->first));
        element->SetCount(p->second);
    }
}

void NRTYServer::THistogram::Serialize(NJson::TJsonValue& json) const {
    json["Name"] = GetName();
    json["Range"] = GetRange();

    NJson::TJsonValue& values = json["Values"];
    for (auto p = Values.begin(); p != Values.end(); ++p) {
        NJson::TJsonValue value;
        value["Start"] = p->first;
        value["End"] = GetRangeEnd(p->first);
        value["Count"] = p->second;
        values.AppendValue(value);
    }
}

void NRTYServer::THistogram::Deserialize(const NRTYServer::NProto::THistogram& proto) {
    Name = proto.GetName();
    Range = proto.GetRange();

    for (size_t i = 0; i < proto.ElementSize(); ++i) {
        const auto& element = proto.GetElement(i);
        const ui64 start = element.GetStart();
        const ui64 end = element.GetEnd();
        if (end - start + 1 != Range) {
            throw yexception() << "incorrect histogram element: " << start << "-" << end;
        }

        const ui64 count = element.GetCount();
        Values[start] = count;
    }
}

void NRTYServer::THistogram::Deserialize(const NJson::TJsonValue& json) {
    Name = json["Name"].GetStringRobust();
    Range = json["Range"].GetUIntegerRobust();

    Values.clear();
    for (auto&& value : json["Values"].GetArray()) {
        const ui64 start = value["Start"].GetUIntegerRobust();
        const ui64 end = value["End"].GetUIntegerRobust();
        if (end - start + 1 != Range) {
            throw yexception() << "incorrect histogram element: " << value.GetStringRobust();
        }

        const ui64 count = value["Count"].GetUIntegerRobust();
        Values[start] = count;
    }
}

NRTYServer::THistogram& NRTYServer::THistograms::AddAttribute(const TString& name, ui64 range) {
    if (Histograms.contains(name)) {
        throw yexception() << "Histogram " << name << " is already registered" << Endl;
    }

    Histograms[name] = MakeHolder<NRTYServer::THistogram>(name, range);
    return GetAttribute(name);
}

NRTYServer::THistogram& NRTYServer::THistograms::AddAttribute(THolder<THistogram>&& histogram) {
    CHECK_WITH_LOG(histogram);
    const TString& name = histogram->GetName();
    if (Histograms.contains(name)) {
        throw yexception() << "Histogram " << name << " is already registered" << Endl;
    }

    Histograms[name] = std::forward<THolder<THistogram>&&>(histogram);
    return GetAttribute(name);
}

NRTYServer::THistogram& NRTYServer::THistograms::GetAttribute(const TString& name) {
    if (!Histograms.contains(name)) {
        throw yexception() << "Histogram " << name << " is not registered" << Endl;
    }

    auto h = Histograms[name].Get();
    CHECK_WITH_LOG(h);
    return *h;
}

bool NRTYServer::THistograms::HasAttribute(const TString& name) {
    return Histograms.contains(name);
}

void NRTYServer::THistograms::Merge(const THistograms& other) {
    for (auto&& h : other.Histograms) {
        const NRTYServer::THistogram& otherHistogram = *h.second;
        const TString& name = otherHistogram.GetName();

        auto& histogram = HasAttribute(name) ? GetAttribute(name) : AddAttribute(name, otherHistogram.GetRange());
        histogram.Merge(otherHistogram);
    }
}

void NRTYServer::THistograms::Serialize(const TString& file) const {
    NRTYServer::NProto::THistograms proto;
    Serialize(proto);
    SerializeToTextFormat(proto, file);
}

void NRTYServer::THistograms::Serialize(NJson::TJsonValue& json) const {
    for (auto&& h : Histograms) {
        CHECK_WITH_LOG(h.second);
        NJson::TJsonValue element;
        h.second->Serialize(element);
        json.AppendValue(element);
    }
}

void NRTYServer::THistograms::Serialize(NRTYServer::NProto::THistograms& proto) const {
    for (auto&& h : Histograms) {
        CHECK_WITH_LOG(h.second);
        auto histogram = proto.AddHistogram();
        h.second->Serialize(*histogram);
    }
}

void NRTYServer::THistograms::Deserialize(const TString& file) {
    NRTYServer::NProto::THistograms proto;
    ParseFromTextFormat(file, proto);
    Deserialize(proto);
}

void NRTYServer::THistograms::Deserialize(const NJson::TJsonValue& json) {
    for (auto&& element : json.GetArray()) {
        auto h = MakeHolder<NRTYServer::THistogram>();
        h->Deserialize(element);
        AddAttribute(std::move(h));
    }
}

void NRTYServer::THistograms::Deserialize(const NRTYServer::NProto::THistograms& proto) {
    for (size_t i = 0; i < proto.HistogramSize(); ++i) {
        auto h = MakeHolder<NRTYServer::THistogram>();
        h->Deserialize(proto.GetHistogram(i));
        AddAttribute(std::move(h));
    }
}
