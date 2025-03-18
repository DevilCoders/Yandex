#include "metriclist.h"

#include <dict/dictutil/xml_writer.h>

namespace NSnippets {

TMetricArray::TMetricArray() {
    Reset();
}

void TMetricArray::Reset() {
    memset(Values, 0, sizeof(Values));
}

void TMetricArray::SetValue(EMetricName name, double value) {
    TMetric& v = Values[name];
    v.Value = value;
    v.Count = 1;
}

void TMetricArray::SumValue(EMetricName name, double value) {
    TMetric& v = Values[name];
    v.Value += value;
    ++v.Count;
}

i32 TMetricArray::Count(EMetricName name) const {
    return Values[name].Count;
}

double TMetricArray::Value(EMetricName name) const {
    return Values[name].Value;
}

void TMetricArray::WriteAsXml(const TString& description, IOutputStream* out) const {
    TXmlWriter w(CODES_UTF8, out);
    w.StartDocument();
    w.StartElement("day");
    w.Attribute("date", description);
    w.Text("\n");
    for (EMetricName metric = MN_IS_EMPTY; metric < MN_COUNT; metric = EMetricName(metric + 1)) {
        if (Count(metric) == 0)
            continue;
        w.StartElement("point");
        w.Attribute("system", ToString(metric));
        w.Attribute("value", Value(metric) / Count(metric));
        w.Attribute("size", Count(metric));
        w.EndElement();
        w.Text("\n");
    }
    w.EndElement();
    w.EndDocument();
}

}
