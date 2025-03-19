#pragma once

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/logger/global/global.h>
#include <util/memory/blob.h>
#include <util/random/random.h>

class TFsPath;

void InitLog(int level = 7);

namespace NRTYArchive {
    void Clear(const TFsPath& archive);

    class TArchivePartThreadSafe;

    ui64 PutDocumentWithCheck(TArchivePartThreadSafe&, const TBlob&, ui32 docid);
}

namespace NRTYArchiveTest {
    template <class TKey = TString>
    struct TTestData {
        TKey Key;
        TBlob Value;

        TTestData() = default;

        TTestData(const TKey& key, TBlob val)
            : Key(key)
            , Value(val)
        {}
    };

    class TStrKeyGenerator {
    public:
        using TKeyType = TString;

        TString GenerateKey() const {
            TString key = ::ToString(Id++);
            return key;
        }

    private:
        mutable ui64 Id = 0;
    };

    class TUI32KeyGenerator {
    public:
        using TKeyType = ui32;

        ui32 GenerateKey() const {
            return Id++;
        }

    private:
        mutable ui64 Id = 0;
    };

    class TDefaultBlobGenerator {
    public:
        TDefaultBlobGenerator(ui32 size, bool randomVal)
            : Size(size)
            , RandomVal(randomVal)
        {}

        TBlob GenerateData() const {
            ui32 length = Size;
            if (Size == 0) {
                length = RandomNumber<ui32>(3000) + 1;
            }
            if (RandomVal) {
                char symb = 97 + RandomNumber<ui8>(26);
                return TBlob::FromString(TString(length, symb));
            } else {
                return TBlob::FromString(TString(length, 'd'));
            }
        }

        ui32 GetSize() const {
            CHECK_WITH_LOG(Size != 0);
            return Size;
        }
    private:
        const ui32 Size = 0;
        bool RandomVal;
    };

    class TSizeByCharBlobGenerator {
    public:
        TSizeByCharBlobGenerator(ui32 /*size*/, bool /*randomVal*/)
        {}

        TBlob GenerateData() const {
            char symb = 97 + RandomNumber<ui8>(26);
            ui32 length = SizeBySymb(symb);
            return TBlob::FromString(TString(length, symb));
        }

        static ui32 SizeBySymb(char symb) {
            return 10 * (ui32)symb;
        }
    };

    template<class TKeyGenerator, class TBlobGenerator = TDefaultBlobGenerator>
    class TTestDataGenerator {
    public:
        using TDataSet = TVector<TTestData<typename TKeyGenerator::TKeyType>>;

        TTestDataGenerator(ui32 size = 0, bool randomVal = true)
            : DataGenerator(size, randomVal)
        {}

        void Generate(ui32 docsCount, TDataSet& data) {
            data.clear();
            data.reserve(docsCount);

            for (ui32 i = 0; i < docsCount; ++i) {
                data.push_back(TTestData<typename TKeyGenerator::TKeyType>(KeyGenerator.GenerateKey(), DataGenerator.GenerateData()));
            }
        }

    private:
        TKeyGenerator KeyGenerator;
        TBlobGenerator DataGenerator;
    };

    using TArchiveDataGen = TTestDataGenerator<TUI32KeyGenerator>;
}
