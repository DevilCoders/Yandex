#include <kernel/dssm_applier/embeddings_transfer/ut/applier/consts.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>
#include <util/stream/file.h>
#include <util/string/cast.h>

#include <optional>
#include <utility>

using namespace std;

struct TBlock {
    TString Encoded;
    TVector<float> Decoded;
};

optional<TBlock> ReadBlock(IInputStream& in);
bool CompareBlocks(const TString &prefix, TBlock const& left, TBlock const& right);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        Cerr << "Arguments required: [path_to_canonical] [path_to_test_result]" << Endl;
        return 1;
    }

    TFileInput canonicalInput(argv[1]);
    TFileInput testInput(argv[2]);

    bool hasDiff = false;

    for (auto i=0;;++i) {
        auto left = ReadBlock(canonicalInput);
        auto right = ReadBlock(testInput);
        if (!left.has_value()) {
            if (right.has_value()) {
                hasDiff = true;
                int j = 0;
                do {
                    ++j;
                    right = ReadBlock(testInput);
                } while (right.has_value());
                Cout << j << " tests added" << Endl;
            }
            break;
        }

        if (!right.has_value()) {
            hasDiff = true;
            int j = 0;
            do {
                ++j;
                left = ReadBlock(canonicalInput);
            } while (left.has_value());
            Cout << j << " tests skipped" << Endl;
            break;
        }
        hasDiff |= CompareBlocks(TString("Test ") + ToString(i), left.value(), right.value());
    }
    return hasDiff ? 1 : 0;
}

optional<TBlock> ReadBlock(IInputStream& in) {
    TString encoded;
    if (!in.ReadLine(encoded))
        return {};
    for (;;) {
        TString v;
        if (!in.ReadLine(v))
            break;
        if (v == ResultsDelimiter)
            break;
        encoded.append(v);
    }
    TVector<float> decoded;
    for (;;) {
        TString v;
        if (!in.ReadLine(v))
            break;
        if (v == TestsDelimiter)
            break;
        decoded.push_back(FromString<float>(v));
    }
    return make_optional<TBlock>({move(encoded), move(decoded)});
}

template<typename T>
void PrintChange(TStringBuf type, T const& left, T const& right) {
    Cout << '\t' << type << ": [" << left << "] -> [" << right << ']' << Endl;
}

bool CompareBlocks(const TString &prefix, TBlock const& left, TBlock const& right) {
    if (left.Encoded != right.Encoded) {
        PrintChange(prefix + TString(" encoded"), left.Encoded, right.Encoded);
        return true;
    }

    if (left.Decoded.size() != right.Decoded.size()) {
        PrintChange(prefix + TString(" decoded size"), left.Decoded.size(), right.Decoded.size());
        return true;
    }

    constexpr float epsilon = 1e-6;
    bool hasDiff = false;
    for (size_t i = 0; i < left.Decoded.size(); ++i) {
        if (abs(left.Decoded[i] - right.Decoded[i]) < epsilon)
            continue;
        if (!hasDiff) {
            hasDiff = true;
            Cout << '\t' << prefix << Endl;
        }
        PrintChange(TString("Decoded @") + ToString(i), left.Decoded[i], right.Decoded[i]);
    }
    return hasDiff;
}
