#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include "factor_names.h"

#include <util/folder/filelist.h>
#include <util/system/fs.h>

using namespace NAntiRobot;

Y_UNIT_TEST_SUITE(TFactorNames) {
    Y_UNIT_TEST(FactorName) {
        const auto instance = TFactorNames::Instance();

        UNIT_ASSERT_EXCEPTION(instance->GetFactorNameByIndex(TAllFactors::AllFactorsCount()), yexception);

        UNIT_ASSERT_STRINGS_EQUAL(instance->GetFactorNameByIndex(0), TAllFactors::GetFactorNameByIndex(0));
        UNIT_ASSERT_STRINGS_EQUAL(instance->GetFactorNameByIndex(1), TAllFactors::GetFactorNameByIndex(1));
        UNIT_ASSERT_STRINGS_EQUAL(instance->GetFactorNameByIndex(123), TAllFactors::GetFactorNameByIndex(123));
        UNIT_ASSERT_STRINGS_EQUAL(instance->GetFactorNameByIndex(456), TAllFactors::GetFactorNameByIndex(456));
    }

    Y_UNIT_TEST(FactorsCount) {
        UNIT_ASSERT_VALUES_EQUAL(TFactorNames::Instance()->FactorsCount(), TAllFactors::AllFactorsCount());
    }

    Y_UNIT_TEST(FactorIndex) {
        const auto instance = TFactorNames::Instance();

        UNIT_ASSERT_EXCEPTION(instance->GetFactorIndexByName("nonexistent_factor"), yexception);

        UNIT_ASSERT_VALUES_EQUAL(instance->GetFactorIndexByName(instance->GetFactorNameByIndex(0)), 0);
        UNIT_ASSERT_VALUES_EQUAL(instance->GetFactorIndexByName(instance->GetFactorNameByIndex(2)), 2);
        UNIT_ASSERT_VALUES_EQUAL(instance->GetFactorIndexByName(instance->GetFactorNameByIndex(100)), 100);
        UNIT_ASSERT_VALUES_EQUAL(instance->GetFactorIndexByName(instance->GetFactorNameByIndex(200)), 200);
    }

    Y_UNIT_TEST(TryGetFactorIndex) {
        const auto instance = TFactorNames::Instance();
        size_t index;

        UNIT_ASSERT(!instance->TryGetFactorIndexByName("nonexistent_factor", index));

        UNIT_ASSERT(instance->TryGetFactorIndexByName(instance->GetFactorNameByIndex(0), index));
        UNIT_ASSERT_VALUES_EQUAL(index, 0);

        UNIT_ASSERT(instance->TryGetFactorIndexByName(instance->GetFactorNameByIndex(3), index));
        UNIT_ASSERT_VALUES_EQUAL(index, 3);

        UNIT_ASSERT(instance->TryGetFactorIndexByName(instance->GetFactorNameByIndex(11), index));
        UNIT_ASSERT_VALUES_EQUAL(index, 11);

        UNIT_ASSERT(instance->TryGetFactorIndexByName(instance->GetFactorNameByIndex(111), index));
        UNIT_ASSERT_VALUES_EQUAL(index, 111);
    }

    TFsPath GetLastFactorsFilePath() {
        auto factorsDirectoryPath = TFsPath(ArcadiaSourceRoot()) / "antirobot/daemon_lib/factors_versions/";
        if (factorsDirectoryPath.IsSymlink()) {
            factorsDirectoryPath = NFs::ReadLink(factorsDirectoryPath);
        }
        UNIT_ASSERT(!factorsDirectoryPath.IsSymlink());

        TFileList fileList;
        fileList.Fill(factorsDirectoryPath, "factors_", false);
        TVector<TString> files;
        while (auto file = fileList.Next()) {
            files.push_back(file);
        }

        auto getFactorsVersion = [](TStringBuf name) {
            UNIT_ASSERT(name.SkipPrefix("factors_"));
            size_t idx = 0;
            while (IsDigit(name[idx])) {
                idx++;
            }
            size_t number;
            UNIT_ASSERT(TryFromString(name.Head(idx), number));
            return number;
        };

        return factorsDirectoryPath / *MaxElementBy(files, getFactorsVersion);
    }

    TVector<TString> ReadFactorsFromFile(const TFsPath& filePath) {
        TFileInput fileInput(filePath);
        TVector<TString> result;
        TString line;
        while (fileInput.ReadLine(line)) {
            auto buf = TStringBuf(line);
            UNIT_ASSERT(buf.SkipPrefix("\""));
            if (buf.EndsWith(',')) {
                UNIT_ASSERT(buf.ChopSuffix(","));
            }
            UNIT_ASSERT(buf.ChopSuffix("\""));
            result.push_back(TString{buf});
        }
        return result;
    }

    TVector<TString> GetFactorNamesFromCode() {
        const TFactorNames* factorNames = TFactorNames::Instance();
        TVector<TString> result;
        for (size_t i = 0; i < factorNames->FactorsCount(); ++i) {
            result.push_back(factorNames->GetFactorNameByIndex(i));
        }
        return result;
    }

    Y_UNIT_TEST(CompareWithFile) {
        auto lastFilePath = GetLastFactorsFilePath();
        auto factorsFromFile = ReadFactorsFromFile(lastFilePath);
        auto factorsFromCode = GetFactorNamesFromCode();

        UNIT_ASSERT_VALUES_EQUAL(factorsFromFile.size(), factorsFromCode.size());

        for (size_t i = 0; i < factorsFromFile.size(); i++) {
            UNIT_ASSERT_STRINGS_EQUAL_C(factorsFromFile[i], factorsFromCode[i], " factor by index " << i);
        }
    }
}
