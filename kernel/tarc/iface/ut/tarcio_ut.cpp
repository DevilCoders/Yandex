#include <kernel/tarc/iface/tarcio.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/resource/resource.h>

#include <util/folder/dirut.h>
#include <util/stream/file.h>
#include <util/system/tempfile.h>

Y_UNIT_TEST_SUITE(TMakeArchiveDirSuite) {
    Y_UNIT_TEST(TestMakeArchiveDir) {
        TTempFile arcFile(MakeTempName(nullptr, "indexarc."));
        TTempFile dirFile(MakeTempName(nullptr, "indexdir."));

        TFileOutput(arcFile.Name()).Write(NResource::Find("/input_indexarc"));

        MakeArchiveDir(arcFile.Name(), dirFile.Name());

        const TString canonicalDir = NResource::Find("/canonical_indexdir");
        const TString actualDir = TIFStream(dirFile.Name()).ReadAll();
        UNIT_ASSERT_VALUES_EQUAL(canonicalDir, actualDir);
    }

    Y_UNIT_TEST(TestArchiveDirBuilder) {
        TTempFile arcFile(MakeTempName(nullptr, "indexarc."));
        TFileOutput(arcFile.Name()).Write(NResource::Find("/input_indexarc"));

        TStringStream ss;
        TArchiveDirBuilder dirCreator(ss);

        TArchiveIterator it;
        it.Open(arcFile.Name().data());
        while (const TArchiveHeader* header = it.NextAuto()) {
            dirCreator.AddEntry(*header);
        }
        dirCreator.Finish();

        const TString canonicalDir = NResource::Find("/canonical_indexdir");
        const TString actualDir = ss.Str();
        UNIT_ASSERT_VALUES_EQUAL(canonicalDir, actualDir);
    }

    Y_UNIT_TEST(TestReadFailure) {
        TArchiveIterator it;
        auto tmpdir = GetSystemTempDir();
        UNIT_ASSERT_EXCEPTION(
            it.Open(tmpdir.data()), // crashes if I/O errors are left unchecked
            yexception);
    }
}
