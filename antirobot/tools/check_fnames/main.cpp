#include <antirobot/daemon_lib/cacher_factors.h>
#include <antirobot/idl/antirobot.ev.pb.h>

#include <util/generic/algorithm.h>


using namespace NAntiRobot;

TSet<TString> FactorsFromEnum() {
    TSet<TString> names;

    for (size_t i = 0; i < static_cast<size_t>(ECacherFactors::NumStaticFactors); ++i) {
        names.insert(ToString(static_cast<ECacherFactors>(i)));
    }

    return names;
}

TSet<TString> FactorsFromProto() {
    TSet<TString> names;
    const auto* descriptor = NAntirobotEvClass::TCacherFactors::descriptor();
    for (int i = 0; i <= descriptor->field_count(); ++i) {
        if (const auto* field = descriptor->FindFieldByNumber(i); field) {
            names.insert(field->name());
        }
    }
    return names;
}

TSet<TString> diff(const TSet<TString>& a, const TSet<TString>& b) {
    TSet<TString> difference;
    SetDifference(
            begin(a), end(a),
            begin(b), end(b),
            std::inserter(difference, begin(difference)));

    return difference;
}

int main() {
    const auto factorsFromEnum = FactorsFromEnum();
    const auto factorsFromProto = FactorsFromProto();
    const auto diffEnum = diff(factorsFromEnum, factorsFromProto);

    TSet<TString> protoExtFields{
        "FormulaResult",
        "FormulaThreshold",
        "Header",
        "LastVisits",
    };

    const auto diffProto = diff(factorsFromProto, factorsFromEnum);
    const auto diffProtoWithoutExtFields = diff(diffProto, protoExtFields);


    if (diffEnum.size() || diffProtoWithoutExtFields.size()) {
        for (auto& name: diffEnum) {
            Cerr << "-" << name << "\n";
        }
        for (auto& name: diffProtoWithoutExtFields) {
            Cerr << "+" << name << "\n";
        }
        return 1;
    }

    return 0;
}

