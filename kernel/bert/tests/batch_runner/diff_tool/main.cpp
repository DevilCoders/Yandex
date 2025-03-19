#include <kernel/bert/tests/batch_runner/consts.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/string/cast.h>

#include <cmath>
#include <optional>

using namespace std;

struct TBlock {
    TString Head;
    TVector<float> Data;
};

optional<TBlock> ReadBlock(IInputStream& in);
bool CompareBlocks(TBlock const& left, TBlock const& right);
void PrintChangeHeader(TString const& head);

int main(int argc, char const* argv[]) {
    if (argc < 3) {
        Cerr << "Arguments required: [path_to_canonical] [path_to_test_result]" << Endl;
        return 1;
    }

    TFileInput canonicalInput(argv[1]);
    TFileInput testInput(argv[2]);

    bool hasDiff = false;

    for (;;) {
        auto left = ReadBlock(canonicalInput);
        auto right = ReadBlock(testInput);
        if (!left.has_value()) {
            if (right.has_value()) {
                hasDiff = true;
                do {
                    Cout << "Block added: [" << right->Head << "]" << Endl;
                    right = ReadBlock(testInput);
                } while (right.has_value());
            }
            break;
        }

        if (!right.has_value()) {
            hasDiff = true;
            do {
                Cout << "Block removed: [" << left->Head << "]" << Endl;
                left = ReadBlock(canonicalInput);
            } while (left.has_value());
            break;
        }
        hasDiff |= CompareBlocks(left.value(), right.value());
    }

    return hasDiff ? 1 : 0;
}

optional<TBlock> ReadBlock(IInputStream& in) {
    TString head;
    if (!in.ReadLine(head)) {
        return {};
    }
    TVector<float> data;
    for (;;) {
        TString v;
        if (!in.ReadLine(v)) {
            break;
        }
        if (v == ResultsDelimiter) {
            break;
        }
        data.push_back(FromString<float>(v));
    }
    return make_optional<TBlock>({move(head), move(data)});
}

template<typename T>
void PrintChange(TStringBuf type, T const& left, T const& right) {
    Cout << "\t" << type << ": [" << left << "] -> [" << right << "]" << Endl;
}

bool CompareBlocks(TBlock const& left, TBlock const& right) {
    if (left.Head != right.Head) {
        PrintChangeHeader(left.Head);
        PrintChange("head", left.Head, right.Head);
        return true;
    }

    if (left.Data.size() != right.Data.size()) {
        PrintChangeHeader(left.Head);
        PrintChange("size", left.Data.size(), right.Data.size());
        return true;
    }

    constexpr float epsilon = 0.0001f;
    bool hasDataDiff = false;
    for (size_t i = 0; i < left.Data.size(); ++i) {
        if (abs(left.Data[i] - right.Data[i]) < epsilon) {
            continue;
        }
        if (!hasDataDiff) {
            hasDataDiff = true;
            PrintChangeHeader(left.Head);
        }
        Cout << "\tdata @" << i << ": [" << left.Data[i] << "] -> [" << right.Data[i] << "]" << Endl;
    }
    return hasDataDiff;
}

void PrintChangeHeader(TString const& head) {
    Cout << "Block changed: head = [" << head << "]" << Endl;
}
