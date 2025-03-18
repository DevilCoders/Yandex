#include "archive_writer.h"
#include "archive_iterator_ut.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/folder/filelist.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/system/fs.h>
#include <util/memory/blob.h>

using namespace NTar;

class TArchiveWriterTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TArchiveWriterTest);
    UNIT_TEST(TestTarArchiveWriter);
    UNIT_TEST(TestTgzArchiveWithSymlinks);
    UNIT_TEST_SUITE_END();
private:
    void TestTarArchiveWriter();
    void TestTgzArchiveWithSymlinks();

    void Pack(const TString& NewArchivePath, const TString& unpackedPath);
};

UNIT_TEST_SUITE_REGISTRATION(TArchiveWriterTest);

void TArchiveWriterTest::TestTgzArchiveWithSymlinks() {
    TFsPath testDir("./symlink_data");
    testDir.MkDir();
    TFsPath filePath(testDir / "1.so.1.2.3");
    TFileOutput file(filePath);
    file.Write("123");
    TFsPath link1(testDir / "1.so.1");
    NFs::SymLink(filePath.Basename(), link1);
    TFsPath link2(testDir / "1.so");
    NFs::SymLink(link1.Basename(), link2);
    Pack("./symlink_data.tar.gz", testDir);
    AssertEqualArchiveUnittest("./symlink_data.tar.gz", testDir);
}

void TArchiveWriterTest::TestTarArchiveWriter() {
    Pack("./new_test_data.tar", "./test_data/");
    AssertEqualArchiveUnittest("./new_test_data.tar", "./test_data/");
}

void TArchiveWriterTest::Pack(const TString& NewArchivePath, const TString& unpackedPath) {
    TArchiveWriter writer(NewArchivePath);
    TFileList unpackedFiles;
    TDirsList unpackedDirs;
    unpackedFiles.Fill(unpackedPath, TStringBuf(), TStringBuf(), -1, false);
    unpackedDirs.Fill(unpackedPath, TStringBuf(), TStringBuf(), -1, false);
    const char* path = unpackedFiles.Next();
    while (path) {
        if (TFsPath(path).IsSymlink()) {
            writer.WriteSymlink(path, TFsPath(path).ReadLink());
        } else {
            TBlob blob = TBlob::FromFile(unpackedPath + "/" + path);
            writer.WriteFile(path, blob);
        }
        path = unpackedFiles.Next();
    }
    path = unpackedDirs.Next();
    while (path) {
        writer.WriteDir(path);
        path = unpackedDirs.Next();
    }
}
