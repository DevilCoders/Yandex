#include "archive_iterator.h"
#include "archive_iterator_ut.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/folder/filelist.h>
#include <util/folder/path.h>
#include <util/generic/set.h>
#include <util/stream/file.h>

using namespace NTar;

class TArchiveIteratorTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TArchiveIteratorTest);
    UNIT_TEST(TestTarArchive);
    UNIT_TEST_SUITE_END();
private:
    void TestTarArchive();
};

UNIT_TEST_SUITE_REGISTRATION(TArchiveIteratorTest);

void TArchiveIteratorTest::TestTarArchive() {
    AssertEqualArchiveUnittest("./test_data.tar", "./test_data/", false);
    AssertEqualArchiveUnittest("./test_data.tar", "./test_data/", true);
}

void AssertEqualArchiveUnittest(const TString& archivePath, const TString& unpackedPath, bool readFiles) {
    TFileList unpackedFiles;
    TDirsList unpackedDirs;
    unpackedFiles.Fill(unpackedPath, TStringBuf(), TStringBuf(), -1, true);
    unpackedDirs.Fill(unpackedPath, TStringBuf(), TStringBuf(), -1, true);

    TArchiveIterator archiveIterator(archivePath);
    TSet<TString> discoveredFiles;
    TSet<TString> discoveredDirs;
    auto prev_iter = archiveIterator.end();
    bool firstIteration = true;
    for (auto iter = archiveIterator.begin(); iter != archiveIterator.end(); ++iter) {
        if (firstIteration) {
            firstIteration = false;
        } else {
            UNIT_ASSERT_EXCEPTION(prev_iter->GetPath(), yexception); // On first iteration that would call std::abort
        }
        prev_iter = iter;

        UNIT_ASSERT_NO_DIFF(iter->GetPath(), iter->GetPathUTF8());
        UNIT_ASSERT(!iter->IsSymLink()); // SymLinks to be implemented
        if (iter->IsSymLink()) {
            UNIT_ASSERT_VALUES_EQUAL(iter->GetType(), AFT_SYMLINK);
            discoveredFiles.insert(iter->GetPath());
            TString target = iter->GetSymLink();
            TString correct = TFsPath(unpackedPath + "/" + iter->GetPath()).ReadLink();
            UNIT_ASSERT(iter->GetStream().ReadAll().Empty());
        } else if (iter->IsRegular()) {
            UNIT_ASSERT_VALUES_EQUAL(iter->GetType(), AFT_REGULAR);
            discoveredFiles.insert(iter->GetPath());
            if (readFiles) {
                TString data = iter->GetStream().ReadAll();
                TString correct = TFileInput(unpackedPath + "/" + iter->GetPath()).ReadAll();
                UNIT_ASSERT_VALUES_EQUAL(data.Size(), iter->GetSize());
                UNIT_ASSERT(data == correct);
            }
        } else {
            UNIT_ASSERT(iter->IsDir());
            UNIT_ASSERT_VALUES_EQUAL(iter->GetType(), AFT_DIR);
            discoveredDirs.insert(iter->GetPath());
            UNIT_ASSERT(iter->GetStream().ReadAll().Empty());
        }
    }
    UNIT_ASSERT_VALUES_EQUAL(discoveredFiles.size(), unpackedFiles.Size());
    UNIT_ASSERT_VALUES_EQUAL(discoveredDirs.size(), unpackedDirs.Size());
}
