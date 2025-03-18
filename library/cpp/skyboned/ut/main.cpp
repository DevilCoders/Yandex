#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/skyboned/manager.h>

#include <util/system/env.h>
#include <util/string/hex.h>

using namespace NSkyboneD;

const TString ExpectedRBTorrent = "rbtorrent:cd67988095492d8cfae40033662a9c4aac5653ae";

Y_UNIT_TEST_SUITE(libskyboned) {

    Y_UNIT_TEST(MakeTorrent) {
        TString fullData;
        for (int i = 0; i < 1024 * 1024; i++) {
            fullData += "some big and nice content"; // about 26 mb of data
        }

        NSkyboneD::TSkyShare share("invalid-tvm-ticket", {});

        auto file1 = share.AddFile();
        file1->SetExecutable(false);
        file1->SetPath("file1");
        file1->SetPublicHttpLink("https://saas2-content-yabs-testing.s3.mds.yandex.net/test_skyboned_file1");
//        file1->Write("some content");
//        file1->Finish();
        file1->SetFingerprint(12, "9893532233caff98cd083a116b013c0b", HexDecode("94e66df8cd09d410c62d9e0dc59d3a884e458e05"));

        auto file2 = share.AddFile();
        file2->SetExecutable(false);
        file2->SetPath("path/to/deep/file2");
        file2->SetPublicHttpLink("https://saas2-content-yabs-testing.s3.mds.yandex.net/test_skyboned_file2");
        file2->Write(fullData);
        file2->Finish();

        share.AddFile("file3", "", true)->Finish();

        auto file4 = share.AddFile();
        file4->SetExecutable(false);
        file4->SetPath("path/file4");
        file4->SetPublicHttpLink("https://saas2-content-yabs-testing.s3.mds.yandex.net/test_skyboned_file4");
        for (int i = 0; i < 8 * 1024 * 1024; i++) { // == 8 MiB, exactly 2 torrent blocks
            file4->Write(static_cast<char>(i));
        }
        file4->Finish();

        share.AddEmptyDirectory("empty/dir");

        share.AddSymlink("path/to/deep/file2", "symlink");

        const TString& rbtorrent = share.GetTemporaryRBTorrentWithoutSkybonedRegistration();
        UNIT_ASSERT_VALUES_EQUAL(rbtorrent, ExpectedRBTorrent);
    }

}
