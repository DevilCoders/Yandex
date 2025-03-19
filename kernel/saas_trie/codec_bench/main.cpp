#include <kernel/saas_trie/idl/trie_key.h>
#include <kernel/qtree/compressor/factory.h>

#include <library/cpp/getopt/small/last_getopt.h>

#include <util/datetime/base.h>
#include <util/generic/deque.h>
#include <util/stream/file.h>
#include <util/string/split.h>

constexpr size_t BATCH_SIZE = 1000;

struct TConfig {
    TString KeysPath;

    void Print(IOutputStream& out) {
        out << "--- Config --\n";
        out << "KeysPath: " << KeysPath << '\n';
        out << "---" << Endl;
    }

    void Init(int argc, const char* argv[]) {
        auto makeOpt = [](TString name, TString help, bool required = true) {
            NLastGetopt::TOpt opt;
            opt.AddLongName(name);
            opt.Help(help);
            if (required) {
                opt.Required();
            }
            return opt;
        };
        NLastGetopt::TOpts opts;
        opts.AddHelpOption('h');
        opts.AddOption(makeOpt("keys", "path to keys", true).StoreResult(&KeysPath));
        opts.AddHelpOption('h');
        NLastGetopt::TOptsParseResult parser(&opts, argc, argv);
    }
};

TDeque<TString> LoadKeys(const TString& path) {
    TDeque<TString> result;
    auto content = TIFStream(path).ReadAll();
    for (const auto& it : StringSplitter(content).Split('\n').SkipEmpty()) {
        TString strKey{it.Token()};
        result.emplace_back(std::move(strKey));
    }
    return result;
}

struct TAggregator {
    void Push(ui64 value) {
        Sum += value;
        ++Count;
    }
    double GetAverage() const {
        return double(Sum) / Count;
    }
    ui64 GetSum() const {
        return Sum;
    }

private:
    ui64 Sum = 0;
    ui64 Count = 0;
};

struct TBatchTester {
    void Test(TCompressorFactory::EFormat format, const TDeque<TString>& data, size_t from) {
        TVector<NSaasTrie::TComplexKey> keys(BATCH_SIZE);
        TVector<NSaasTrie::TComplexKey> keys2(BATCH_SIZE);
        TVector<TString> strKeys(BATCH_SIZE);
        for (size_t i = 0; i < BATCH_SIZE; ++i) {
            NSaasTrie::DeserializeFromCgi(keys[i], data[from + i], true);
        }

        auto t0 = TInstant::Now();

        for (size_t i = 0; i < BATCH_SIZE; ++i) {
            strKeys[i] = NSaasTrie::SerializeToCgi(keys[i], format);
            PackedSize.Push(strKeys[i].size());
        }

        auto t1 = TInstant::Now();

        for (size_t i = 0; i < BATCH_SIZE; ++i) {
            NSaasTrie::DeserializeFromCgi(keys2[i], strKeys[i], true);
        }

        auto t2 = TInstant::Now();

        for (size_t i = 0; i < BATCH_SIZE; ++i) {
            TString strKey;
            Y_PROTOBUF_SUPPRESS_NODISCARD keys2[i].SerializeToString(&strKey);
            RawSize.Push(strKey.size());
        }
        PackTime.Push((t1 - t0).MicroSeconds());
        UnpackTime.Push((t2 -  t1).MicroSeconds());
    }

    double GetAveragePackTime() const {
        return PackTime.GetAverage() / BATCH_SIZE;
    }
    double GetAverageUnpackTime() const {
        return UnpackTime.GetAverage() / BATCH_SIZE;
    }
    double GetCompressionRatio() const {
        return PackedSize.GetSum() * 100.0 / RawSize.GetSum();
    }

private:
    TAggregator PackTime;
    TAggregator UnpackTime;
    TAggregator RawSize;
    TAggregator PackedSize;
};

int main(int argc, const char* argv[]) {
    try {
        TConfig config;
        config.Init(argc, argv);
        config.Print(Cout);

        auto keys = LoadKeys(config.KeysPath);

        TCompressorFactory::EFormat formats[] = {
            TCompressorFactory::ZLIB_DEFAULT,
            TCompressorFactory::ZLIB_MINIMAL,
            TCompressorFactory::ZLIB_UNCOMPRESSED,
            TCompressorFactory::GZIP_DEFAULT,
            TCompressorFactory::LZ_LZ4,
            TCompressorFactory::LZ_SNAPPY,
            TCompressorFactory::LZ_FASTLZ,
            TCompressorFactory::LZ_QUICKLZ,
            TCompressorFactory::BC_ZSTD_08_1
        };

        for (auto format : formats) {
            Cout << "--- " << ToString(format) << " ---" << Endl;
            TBatchTester tester;
            for (size_t i = 0; i + BATCH_SIZE <= keys.size(); i += BATCH_SIZE) {
                tester.Test(format, keys, i);
            }
            Cout << "Pack:        " << tester.GetAveragePackTime() << "us\n";
            Cout << "Unpack:      " << tester.GetAverageUnpackTime() << "us\n";
            Cout << "Compression: " << tester.GetCompressionRatio() << "%\n" << Endl;
        }

        return 0;

    } catch (...) {
        Cerr << CurrentExceptionMessage() << Endl;
        return 1;
    }
}

